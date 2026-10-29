#!/usr/bin/env python3
"""The project's PreToolUse guards (E-01). Claude Code passes the tool call as JSON on stdin; exit 2 refuses it and
stderr tells the agent why and what to do instead. Anything this script cannot judge is let through.

- Read: a text file over 100 KB with neither offset nor limit. A whole Read pays the tool's 25k-token cap and still
  shows only part of such a file.
- Bash: a commit, tag, note, push, PR, issue, release or gh api write that carries a claude.ai/code link or a
  Claude-Session trailer, in the command itself or in a message file it names (-F, --file, --body-file, --notes-file,
  --input, field=@file). The user regards the session link as private. Greps and logs are left alone.
- Bash: a `make clean` whose Makefile still removes build/ whole (a checkout older than E-02's Makefile, which removes only
  the objects and the binaries): it would take build/toolchains, build/perf-baselines, build/prove and the proof kits with it.
  And an rm of build/ itself, always.
"""
import json
import os
import re
import shlex
import subprocess
import sys

READ_LIMIT = 100 * 1024
NOT_TEXT = {".png", ".jpg", ".jpeg", ".gif", ".webp", ".bmp", ".ico", ".pdf", ".ipynb", ".wav", ".mp3", ".ogg"}

SESSION = re.compile(r"claude\.ai/code|Claude-Session", re.IGNORECASE)
# a command position (the start, or after ; & | ( or a newline) and any VAR=value prefixes, so prose that names
# "git tag" inside a heredoc or a quoted message is not taken for the command itself
AT_COMMAND = r"(?:^|[;&|(]|\n)\s*(?:\w+=(?:\"[^\"]*\"|'[^']*'|\S*)\s+)*"
GIT_WRITE = re.compile(AT_COMMAND + r"git\b(?:\s+(?:-[cC]\s+\S+|--?[\w-]+(?:=\S+)?))*\s+(commit|tag|notes|push)\b")
GH_WRITE = re.compile(AT_COMMAND + r"gh\s+(?:(?:pr|issue|release)\s+(?:create|edit|comment|review|merge)|api)\b")
GH_API_WRITE = re.compile(r"\s(?:-X|--method)\s*(?:POST|PATCH|PUT)\b|\s(?:-f|-F|--field|--raw-field|--input)[\s=]",
                          re.IGNORECASE)
FILE_FLAG = re.compile(r"(?:^|\s)(?:-F|--file|--body-file|--notes-file|--input|--field)(?:=|\s+)(['\"]?)(\S+?)\1(?=\s|$)")
MAKE_CLEAN = re.compile(AT_COMMAND + r"make\b([^;&|\n]*)\bclean\b")
WHOLE_BUILD = re.compile(r"^clean:[^\n]*\n(?:\t[^\n]*\n?)*?\t[^\n]*\brm\s+-\w*\s+(?:\S+\s+)*?build/?(?:\s|$)", re.M)


def refuse(message):
    print(message, file=sys.stderr)
    sys.exit(2)


def check_read(tool_input, cwd):
    path = tool_input.get("file_path") or ""
    if not path or tool_input.get("offset") is not None or tool_input.get("limit") is not None:
        return
    if os.path.splitext(path)[1].lower() in NOT_TEXT:
        return
    full = path if os.path.isabs(path) else os.path.join(cwd, path)
    try:
        size = os.path.getsize(full)
    except OSError:
        return
    if size > READ_LIMIT:
        refuse(f"Refused: {path} is {size // 1024} KB, and a whole Read stops at 25k tokens showing only part of it. "
               "Read a range (offset and limit), find the place first with grep -n or the headings (grep -n '^#'), "
               "or ask the LSP for the file's symbols. Where things are: CLAUDE.md's map and the directory's CLAUDE.md.")


def named_files(command, cwd):
    """The message files a command names: -F file, --body-file file, field=@file; '-' (stdin) is in the command."""
    for match in FILE_FLAG.finditer(command):
        value = match.group(2)
        if "=@" in value:
            value = value.split("=@", 1)[1]
        elif "=" in value or value == "-":
            continue
        value = os.path.expanduser(os.path.expandvars(value))
        yield value if os.path.isabs(value) else os.path.join(cwd, value)


def carries_session(command, cwd):
    if SESSION.search(command):
        return "the command"
    for path in named_files(command, cwd):
        try:
            with open(path, "r", errors="replace") as f:
                if SESSION.search(f.read(4 * 1024 * 1024)):
                    return path
        except OSError:
            continue
    return None


def git_out(cwd, *args):
    try:
        return subprocess.run(["git", "-C", cwd, *args], capture_output=True, text=True, timeout=10).stdout
    except (OSError, subprocess.SubprocessError):
        return ""


def check_session_links(command, cwd):
    git = GIT_WRITE.search(command)
    gh = GH_WRITE.search(command)
    if gh and gh.group(0).endswith("api") and not GH_API_WRITE.search(command):
        gh = None  # a gh api read
    if not git and not gh:
        return
    verb = git.group(1) if git else None
    if verb == "push":
        # what a push would publish: the commits not yet on origin/main
        found = git_out(cwd, "log", "--format=%h", "-i", "--grep=claude.ai/code", "--grep=Claude-Session",
                        "origin/main..HEAD")
        if found.strip():
            refuse("Refused: commits about to be pushed carry a session link or a Claude-Session trailer ("
                   + ", ".join(found.split()) + "). Rewrite their messages without it first; the session link is "
                   "private and never goes to GitHub.")
        return
    where = carries_session(command, cwd)
    if not where and verb == "commit" and "--amend" in command and re.search(r"--no-edit|\s-C\s|--reuse-message", command):
        if SESSION.search(git_out(cwd, "log", "-1", "--format=%B", "HEAD")):
            where = "HEAD's message, which --amend --no-edit keeps"
    if where:
        refuse(f"Refused: {where} carries a claude.ai/code link or a Claude-Session trailer. The user regards the "
               "session link as private: leave it out of commit messages, tags, PR and issue bodies and release notes "
               "(the Co-Authored-By line is fine).")


def removes_build(command, cwd):
    project = os.environ.get("CLAUDE_PROJECT_DIR") or cwd
    build = os.path.realpath(os.path.join(project, "build"))
    for segment in re.split(r"[;&|\n]+", command):
        try:
            words = shlex.split(segment)
        except ValueError:
            continue
        while words and ("=" in words[0] or words[0] in ("sudo", "command", "nice", "caffeinate", "-i")):
            words = words[1:]
        if not words or words[0] != "rm":
            continue
        for word in words[1:]:
            if word.startswith("-"):
                continue
            target = os.path.expanduser(os.path.expandvars(word))
            target = target if os.path.isabs(target) else os.path.join(cwd, target)
            target = target.rstrip("/")
            if target.endswith("/*"):
                target = target[:-2]
            if os.path.realpath(target) == build:
                return True
    return False


def cleans_whole_build(command, cwd):
    """A make clean whose Makefile (the one in -C's directory, else the working directory's) removes build/ whole, as every
    Makefile before E-02's did: an old checkout or worktree."""
    match = MAKE_CLEAN.search(command)
    if not match:
        return False
    where = re.search(r"-C\s*(\S+)", match.group(1))
    directory = os.path.join(cwd, os.path.expanduser(where.group(1))) if where else cwd
    try:
        with open(os.path.join(directory, "Makefile")) as f:
            return bool(WHOLE_BUILD.search(f.read()))
    except OSError:
        return False


def check_build_files(command, cwd):
    if cleans_whole_build(command, cwd):
        refuse("Refused: this Makefile's clean runs `rm -rf build`, which also deletes build/toolchains (the release "
               "toolchains), build/perf-baselines, build/prove and every milestone's proof kit (a checkout older than E-02). "
               "Delete the object directories by name instead: rm -rf build/core build/galaxy build/space build/surface "
               "build/game build/harness build/*.o build/*.d vesperis vesperis_test")
    if removes_build(command, cwd):
        refuse("Refused: removing build/ itself deletes build/toolchains, build/perf-baselines and the proof kits. "
               "Remove the directories inside it by name instead.")


def main():
    try:
        data = json.load(sys.stdin)
    except ValueError:
        return
    tool = data.get("tool_name", "")
    tool_input = data.get("tool_input") or {}
    cwd = data.get("cwd") or os.getcwd()
    if tool == "Read":
        check_read(tool_input, cwd)
    elif tool == "Bash":
        command = tool_input.get("command") or ""
        check_session_links(command, cwd)
        check_build_files(command, cwd)


if __name__ == "__main__":
    main()
