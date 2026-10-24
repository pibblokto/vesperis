#!/usr/bin/env python3
"""The guard's cases: python3 .claude/hooks/test_guard.py (from anywhere). Each case is (name, tool, input, cwd, the exit
expected); 2 is a refusal. The message files live in a temporary directory."""
import json, os, subprocess, sys, tempfile

G = os.path.join(os.path.dirname(os.path.abspath(__file__)), "guard.py")
P = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
H = tempfile.mkdtemp()
open(H + "/msg_bad.txt", "w").write("Title\n\nCo-Authored-By: Claude <noreply@anthropic.com>\nClaude-Session: https://claude.ai/code/session_x\n")
open(H + "/msg_ok.txt", "w").write("Title\n\nCo-Authored-By: Claude <noreply@anthropic.com>\n")
open(H + "/body_bad.json", "w").write('{"title":"x","body":"see https://claude.ai/code/session_x"}')
OLD, NEW = tempfile.mkdtemp(), tempfile.mkdtemp()   # a Makefile from before E-02 (its clean takes build/ whole) and E-02's
open(OLD + "/Makefile", "w").write("all: x\n\nclean:\n\trm -rf build vesperis vesperis_test\n")
open(NEW + "/Makefile", "w").write("all: x\n\nclean:\n\trm -rf $(OBJ_DIRS) build/*.o build/*.d vesperis vesperis_test\n")
S = "Claude-" + "Session"
L = "claude.ai/" + "code"

cases = [
    ("read harness whole", "Read", {"file_path": P + "/src/main_headless.cpp"}, P, 2),
    ("read harness relative", "Read", {"file_path": "src/main_headless.cpp"}, P, 2),
    ("read harness range", "Read", {"file_path": P + "/src/main_headless.cpp", "offset": 100, "limit": 50}, P, 0),
    ("read small file", "Read", {"file_path": P + "/Makefile"}, P, 0),
    ("read 10-decisions whole", "Read", {"file_path": P + "/docs/reference/10-decisions.md"}, P, 2),
    ("read png", "Read", {"file_path": P + "/shots/tests/x.png"}, P, 0),
    ("read missing", "Read", {"file_path": "/nope/x.txt"}, P, 0),
    ("commit -m with trailer", "Bash", {"command": f'git commit -m "x\n\n{S}: https://{L}/session_x"'}, P, 2),
    ("commit heredoc with link", "Bash", {"command": f"git -c user.name=ab12 -c user.email=x@example.com commit -F - <<'EOF'\nTitle\nhttps://{L}/session_01\nEOF"}, P, 2),
    ("commit -F bad file", "Bash", {"command": f"git commit -F {H}/msg_bad.txt"}, P, 2),
    ("commit -F ok file", "Bash", {"command": f"git commit -F {H}/msg_ok.txt"}, P, 0),
    ("commit --file= bad", "Bash", {"command": f"git commit --file={H}/msg_bad.txt"}, P, 2),
    ("env + commit ok", "Bash", {"command": 'GIT_COMMITTER_NAME=ab GIT_COMMITTER_EMAIL=c@example.com git -c user.name=ab commit -m "Title\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"'}, P, 0),
    ("quoted env + commit bad", "Bash", {"command": f'GIT_AUTHOR_NAME="a b" git commit -m "{S}: y"'}, P, 2),
    ("cd && commit bad", "Bash", {"command": f'cd /tmp && git commit -m "{S}: y"'}, P, 2),
    ("subshell tag bad", "Bash", {"command": f'(git tag -a v1 -m "{L}/x")'}, P, 2),
    ("tag -m with link", "Bash", {"command": f'git tag -a v1.4.0 -m "see {L}/session"'}, P, 2),
    ("gh pr create body-file bad", "Bash", {"command": f"gh pr create --title x --body-file {H}/msg_bad.txt"}, P, 2),
    ("gh pr create -F bad", "Bash", {"command": f"gh pr create --title x -F {H}/msg_bad.txt"}, P, 2),
    ("gh pr create ok", "Bash", {"command": 'gh pr create --title x --body "Generated with Claude Code"'}, P, 0),
    ("gh release notes-file bad", "Bash", {"command": f"gh release create v1 --notes-file {H}/msg_bad.txt"}, P, 2),
    ("gh api POST input bad", "Bash", {"command": f"gh api repos/a/b/pulls -X POST --input {H}/body_bad.json"}, P, 2),
    ("gh api field @file bad", "Bash", {"command": f"gh api repos/a/b/releases -F body=@{H}/msg_bad.txt"}, P, 2),
    ("gh api GET piped grep", "Bash", {"command": f"gh api repos/a/b/commits | grep {S}"}, P, 0),
    ("git log grep", "Bash", {"command": f"git log --grep={S} --format=%h"}, P, 0),
    ("grep for link", "Bash", {"command": f'grep -rn "{L}" docs'}, P, 0),
    ("prose heredoc naming git tag", "Bash", {"command": f"cat >> notes.md <<'EOF'\n* a `git tag` naming the trailer ({S}) and a {L} link were refused\nEOF"}, P, 0),
    ("git status", "Bash", {"command": "git status"}, P, 0),
    ("push (repo state)", "Bash", {"command": "git push origin release/v1.4.0"}, P, 0),
    ("make clean, an old Makefile", "Bash", {"command": "make clean"}, OLD, 2),
    ("make -j8 clean, an old Makefile", "Bash", {"command": "make -j8 clean && make -j8"}, OLD, 2),
    ("make -C old clean", "Bash", {"command": f"make -C {OLD} clean"}, NEW, 2),
    ("make clean, E-02's Makefile", "Bash", {"command": "make clean && make -j8"}, NEW, 0),
    ("prose naming make clean", "Bash", {"command": "cat > x.md <<'EOF'\nall that `make clean` removes\nEOF"}, OLD, 0),
    ("make -j8", "Bash", {"command": "make -j8 2>&1 | tail -3"}, P, 0),
    ("rm -rf build", "Bash", {"command": "rm -rf build"}, P, 2),
    ("rm -rf build/ abs", "Bash", {"command": f"rm -rf {P}/build/"}, P, 2),
    ("rm -rf build/*", "Bash", {"command": "rm -rf build/*"}, P, 2),
    ("rm -rf build/d05/verify", "Bash", {"command": "rm -rf build/d05/verify; mkdir -p build/d05/verify"}, P, 0),
    ("rm build from other cwd", "Bash", {"command": "rm -rf ../build"}, P + "/src", 2),
    ("rm -rf builds (other)", "Bash", {"command": "rm -rf build2 buildx"}, P, 0),
]

bad = 0
for name, tool, inp, cwd, want in cases:
    data = json.dumps({"tool_name": tool, "tool_input": inp, "cwd": cwd})
    r = subprocess.run(["python3", G], input=data, capture_output=True, text=True, env={**os.environ, "CLAUDE_PROJECT_DIR": P})
    ok = r.returncode == want
    bad += not ok
    print(f"{'ok ' if ok else 'BAD'} {name:34} exit {r.returncode} (want {want}) {r.stderr[:50].strip()}")
print(f"{len(cases) - bad} of {len(cases)} as expected")
sys.exit(1 if bad else 0)
