// gfsk8_python.cpp - pybind11 wrapper for gfsk8-modem-clean
// First iteration: full RX path, with stubs for TX path to add later.

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "gfsk8modem.h"
#include "../src/DecodedText.h"

#include <span>
#include <cstdint>
#include <vector>
#include <sstream>

namespace py = pybind11;

PYBIND11_MODULE(gfsk8, m) {
    m.doc() = "Python bindings for gfsk8-modem-clean (JS8/FT8 modem library)";

    // ─── Submode enum ────────────────────────────────────────────────────
    py::enum_<gfsk8::Submode>(m, "Submode")
        .value("Normal", gfsk8::Submode::Normal)
        .value("Fast",   gfsk8::Submode::Fast)
        .value("Turbo",  gfsk8::Submode::Turbo)
        .value("Slow",   gfsk8::Submode::Slow)
        .value("Ultra",  gfsk8::Submode::Ultra)
        .export_values();

    // ─── Bitmask constants ───────────────────────────────────────────────
    m.attr("AllSubmodes") = py::int_(gfsk8::AllSubmodes);
    // Convenience masks (matching the integer values in the Submode enum)
    m.attr("MASK_NORMAL") = py::int_(0x01);
    m.attr("MASK_FAST")   = py::int_(0x02);
    m.attr("MASK_TURBO")  = py::int_(0x04);
    m.attr("MASK_SLOW")   = py::int_(0x10);  // Slow=4 in enum, but bitmask bit 0x10 per AllSubmodes layout
    m.attr("MASK_ULTRA")  = py::int_(0x08);

    // Buffer size constants (from JS8Call convention)
    m.attr("RX_SAMPLE_SIZE") = py::int_(720000);  // 60 seconds at 12 kHz
    m.attr("RX_SAMPLE_RATE") = py::int_(12000);

    // ─── SubmodeParms struct ─────────────────────────────────────────────
    py::class_<gfsk8::SubmodeParms>(m, "SubmodeParms")
        .def_readonly("samples_per_symbol",  &gfsk8::SubmodeParms::samplesPerSymbol)
        .def_readonly("tone_spacing_hz",     &gfsk8::SubmodeParms::toneSpacingHz)
        .def_readonly("period_seconds",      &gfsk8::SubmodeParms::periodSeconds)
        .def_readonly("start_delay_ms",      &gfsk8::SubmodeParms::startDelayMs)
        .def_readonly("num_symbols",         &gfsk8::SubmodeParms::numSymbols)
        .def_readonly("sample_rate",         &gfsk8::SubmodeParms::sampleRate)
        .def_readonly("rx_snr_threshold_db", &gfsk8::SubmodeParms::rxSnrThresholdDb);

    m.def("submode_parms", &gfsk8::submodeParms,
          py::arg("submode"),
          "Return the fixed SubmodeParms for a given Submode.");

    // ─── Decoded struct ──────────────────────────────────────────────────
py::class_<gfsk8::Decoded>(m, "Decoded")
        .def_readonly("message",      &gfsk8::Decoded::message,
                      "Raw 12-character physical-layer payload (JS8 alphabet).")
        .def_readonly("snr_db",       &gfsk8::Decoded::snrDb)
        .def_readonly("frequency_hz", &gfsk8::Decoded::frequencyHz)
        .def_readonly("dt_seconds",   &gfsk8::Decoded::dtSeconds)
        .def_readonly("submode",      &gfsk8::Decoded::submode)
        .def_readonly("quality",      &gfsk8::Decoded::quality)
        .def_readonly("frame_type",   &gfsk8::Decoded::frameType)
        .def_readonly("utc",          &gfsk8::Decoded::utc)
        .def_property_readonly("text", [](const gfsk8::Decoded &d) {
            // Use DecodedText to unpack Varicode and produce human-readable text.
            DecodedText dt(d.message, d.frameType, d.submode);
            return dt.message();
        }, "Human-readable Varicode-decoded message text.")
        .def_property_readonly("is_heartbeat", [](const gfsk8::Decoded &d) {
            DecodedText dt(d.message, d.frameType, d.submode);
            return dt.isHeartbeat();
        })
        .def_property_readonly("is_directed", [](const gfsk8::Decoded &d) {
            DecodedText dt(d.message, d.frameType, d.submode);
            return dt.isDirectedMessage();
        })
        .def_property_readonly("is_compound", [](const gfsk8::Decoded &d) {
            DecodedText dt(d.message, d.frameType, d.submode);
            return dt.isCompound();
        })
        .def_property_readonly("is_low_confidence", [](const gfsk8::Decoded &d) {
            DecodedText dt(d.message, d.frameType, d.submode);
            return dt.isLowConfidence();
        })
        .def("__repr__", [](const gfsk8::Decoded &d) {
            DecodedText dt(d.message, d.frameType, d.submode);
            std::ostringstream s;
            s << "<Decoded submode=" << d.submode
              << " snr=" << d.snrDb << "dB"
              << " freq=" << d.frequencyHz << "Hz"
              << " text='" << dt.message() << "'>";
            return s.str();
        });
    // ─── TxFrame struct ──────────────────────────────────────────────────
    py::class_<gfsk8::TxFrame>(m, "TxFrame")
        .def_readonly("payload",    &gfsk8::TxFrame::payload)
        .def_readonly("frame_type", &gfsk8::TxFrame::frameType)
        .def("__repr__", [](const gfsk8::TxFrame &f) {
            return "<TxFrame frame_type=" + std::to_string(f.frameType) +
                   " payload='" + f.payload + "'>";
        });

    // ─── pack() ──────────────────────────────────────────────────────────
    m.def("pack", &gfsk8::pack,
          py::arg("mycall"),
          py::arg("mygrid"),
          py::arg("text"),
          py::arg("submode") = gfsk8::Submode::Normal,
          "Pack a human-readable JS8Call message into physical-layer frames. "
          "Returns a list of TxFrame objects, each ready for modulate().");

    // ─── modulate() ──────────────────────────────────────────────────────
    m.def("modulate",
          [](gfsk8::Submode submode, int frameType,
             const std::string &message, double audioFrequencyHz) {
              auto pcm = gfsk8::modulate(submode, frameType, message, audioFrequencyHz);
              // Return as a numpy float32 array
              return py::array_t<float>(pcm.size(), pcm.data());
          },
          py::arg("submode"),
          py::arg("frame_type"),
          py::arg("message"),
          py::arg("audio_frequency_hz") = 1500.0,
          "Encode and modulate a message to float32 PCM audio at 12 kHz. "
          "Returns a numpy float32 array including start-delay silence.");

    // ─── encode() (lower-level) ──────────────────────────────────────────
    m.def("encode",
          [](gfsk8::Submode submode, int frameType, const std::string &message) {
              std::vector<int> tones;
              bool ok = gfsk8::encode(submode, frameType, message, tones);
              if (!ok) {
                  throw py::value_error("encode() failed - invalid message characters");
              }
              return tones;  // pybind11 converts vector<int> to list automatically
          },
          py::arg("submode"),
          py::arg("frame_type"),
          py::arg("message"),
          "Encode a 12-character JS8-alphabet message to 79 tone values [0..7].");

    // ─── Decoder class ───────────────────────────────────────────────────
    py::class_<gfsk8::Decoder>(m, "Decoder")
        .def(py::init<int, int, int>(),
             py::arg("submodes") = gfsk8::AllSubmodes,
             py::arg("nfa") = 200,
             py::arg("nfb") = 4000,
             "Construct a Decoder with the given submode bitmask and "
             "frequency search range (nfa = low Hz, nfb = high Hz).")
        .def("decode",
             [](gfsk8::Decoder &self,
                py::array_t<int16_t, py::array::c_style | py::array::forcecast> audio,
                int nutc,
                py::function callback) {
                 auto buf = audio.unchecked<1>();
                 std::span<const int16_t> span(buf.data(0),
                                               static_cast<size_t>(buf.size()));

                 // Release GIL during the heavy DSP work.
                 // The callback re-acquires it inside the lambda.
                 py::gil_scoped_release release;
                 self.decode(span, nutc,
                     [&callback](const gfsk8::Decoded &d) {
                         py::gil_scoped_acquire acquire;
                         callback(d);
                     });
             },
             py::arg("audio"),
             py::arg("nutc"),
             py::arg("callback"),
             "Decode one period's worth of audio. "
             "audio: numpy int16 1-D array, length >= GFSK8_RX_SAMPLE_SIZE. "
             "nutc: UTC timestamp in code_time() encoding. "
             "callback: invoked once per decoded frame with a Decoded object.")
        .def("reset", &gfsk8::Decoder::reset,
             "Reset internal state (clears soft-combining buffers).");

    // ─── Logging ─────────────────────────────────────────────────────────
    m.def("set_log_callback", &gfsk8::setLogCallback,
          py::arg("callback"),
          "Set a callback for internal diagnostic log messages. "
          "Pass None to disable.");
}
