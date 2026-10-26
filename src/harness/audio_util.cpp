// E-03: the sounds' helpers: the receiver's stats and its headless run, a WAV written, an FFT, a spectrum's peak, a sound's pitch
#include "harness/harness.h"
RenderStats renderReceiver(AudioSynth& synth, AudioState& st, double seconds, std::vector<int16_t>* pcm, const std::function<void(double)>& between) {
    const int sr = 22050; std::vector<float> buf(2048); RenderStats r; double sum = 0; long cnt = 0;
    for (int frames = 0; frames < sr * seconds; frames += (int)buf.size()) {
        synth.render(buf.data(), (int)buf.size(), sr, st);
        for (float v : buf) {
            if (!std::isfinite(v)) r.nan++;
            if (std::fabs(v) >= 0.999f) r.clip++;
            r.peak = std::max(r.peak, (double)std::fabs(v));
            sum += (double)v * v;
            cnt++;
            if (pcm) pcm->push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32767));
        }
        if (between) between(buf.size() / (double)sr);
    }
    r.rms = std::sqrt(sum / std::max(1L, cnt));
    return r;
}
bool writeWav16(const std::string& fn, const std::vector<int16_t>& pcm, int sr) {
    FILE* f = fopen(fn.c_str(), "wb"); if (!f) return false;
    uint32_t dataBytes = (uint32_t)pcm.size() * 2, rate = sr, byteRate = sr * 2; uint16_t ch = 1, bits = 16, blockAlign = 2, fmtTag = 1; uint32_t fmtLen = 16, riffLen = 36 + dataBytes;
    fwrite("RIFF", 1, 4, f);
    fwrite(&riffLen, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    fwrite(&fmtLen, 4, 1, f);
    fwrite(&fmtTag, 2, 1, f);
    fwrite(&ch, 2, 1, f);
    fwrite(&rate, 4, 1, f);
    fwrite(&byteRate, 4, 1, f);
    fwrite(&blockAlign, 2, 1, f);
    fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f); fwrite(pcm.data(), 2, pcm.size(), f); fclose(f);
    return true;
}
// D-03: a voice print: the long spectrum of a render in twelfths of an octave from 30 Hz to about 8 kHz (frames of 4,096 under a Hann window,
// half over each other), in dB with its mean taken out (the level does not count); two prints apart by the root of the mean square of their
// difference (dB)
void fftInPlace(std::vector<std::complex<double>>& a) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; i++) { size_t bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap(a[i], a[j]); }
    for (size_t len = 2; len <= n; len <<= 1) {
        const double ang = -TAU / len; const std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1);
            for (size_t j = 0; j < len / 2; j++) {
                const auto u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}
// the strongest frequency of a stretch of the sound between lo and hi Hz (a DFT at 1 Hz over a second)
double spectralPeak(const std::vector<float>& x, double from, double lo, double hi) {
    const int sr = 22050, n = sr, i0 = (int)(from * sr);
    if (i0 + n > (int)x.size()) return 0;
    double best = 0, bestF = 0;
    for (int f = (int)lo; f <= (int)hi; f++) {
        double re = 0, im = 0;
        for (int i = 0; i < n; i++) { const double ph = TAU * f * i / sr; re += x[i0 + i] * std::cos(ph); im += x[i0 + i] * std::sin(ph); }
        const double p = re * re + im * im;
        if (p > best) { best = p; bestF = f; }
    }
    return bestF;
}
double pcmPitch(const std::vector<int16_t>& pcm, int sr, double t0, double t1, double fmin, double fmax) {   // the autocorrelation's best period
    const int i0 = (int)(t0 * sr), i1 = std::min((int)pcm.size(), (int)(t1 * sr));
    if (i1 - i0 < 512) return 0;
    const int lo = (int)(sr / fmax), hi = std::min((int)(sr / fmin), (i1 - i0) / 2);
    std::vector<double> rs(hi + 1, -1); double best = -1;
    for (int l = lo; l <= hi; l++) {
        double c = 0, e1 = 0, e2 = 0;
        for (int i = i0; i + l < i1; i++) {
            c += (double)pcm[i] * pcm[i + l];
            e1 += (double)pcm[i] * pcm[i];
            e2 += (double)pcm[i + l] * pcm[i + l];
        }
        rs[l] = c / std::sqrt(std::max(1.0, e1 * e2));
        best = std::max(best, rs[l]);
    }
    for (int l = lo; l <= hi; l++) if (rs[l] >= 0.92 * best && (l == hi || rs[l] >= rs[l + 1])) return (double)sr / l;   // the shortest period near the best (twice the period correlates as well)
    return 0;
}
