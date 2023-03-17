/********************************************************************
* libavio/src/avio.cpp
*
* Copyright (c) 2023  Stephen Rhodes
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*********************************************************************/

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "avio.h"
#include "Display.h"
#include "Frame.h"

namespace py = pybind11;

namespace avio
{

PYBIND11_MODULE(avio, m)
{
    m.doc() = "pybind11 av plugin";
    py::class_<Player>(m, "Player")
        .def(py::init<>())
        .def("run", &Player::run)
        .def("start", &Player::start)
        .def("seek", &Player::seek)
        .def("isPaused", &Player::isPaused)
        .def("isPiping", &Player::isPiping)
        .def("isRecording", &Player::isRecording)
        .def("isEncoding", &Player::isEncoding)
        .def("setVolume", &Player::setVolume)
        .def("setMute", &Player::setMute)
        .def("togglePaused", &Player::togglePaused)
        .def("togglePiping", &Player::togglePiping)
        .def("toggleEncoding", &Player::toggleEncoding)
        .def("toggleRecording", &Player::toggleRecording)
        .def("getVideoCodec", &Player::getVideoCodec)
        .def("getVideoWidth", &Player::getVideoWidth)
        .def("getVideoHeight", &Player::getVideoHeight)
        .def("getVideoFrameRate", &Player::getVideoFrameRate)
        .def("getVideoBitrate", &Player::getVideoBitrate)
        .def_readwrite("running", &Player::running)
        .def_readwrite("width", &Player::width)
        .def_readwrite("height", &Player::height)
        .def_readwrite("disable_video", &Player::disable_video)
        .def_readwrite("disable_audio", &Player::disable_audio)
        .def_readwrite("progressCallback", &Player::progressCallback)
        .def_readwrite("renderCallback", &Player::renderCallback)
        .def_readwrite("pythonCallback", &Player::pythonCallback)
        .def_readwrite("infoCallback", &Player::infoCallback)
        .def_readwrite("errorCallback", &Player::errorCallback)
        .def_readwrite("cbMediaPlayingStarted", &Player::cbMediaPlayingStarted)
        .def_readwrite("cbMediaPlayingStopped", &Player::cbMediaPlayingStopped)
        .def_readwrite("uri", &Player::uri)
        .def_readwrite("hWnd", &Player::hWnd)
        .def_readwrite("vpq_size", &Player::vpq_size)
        .def_readwrite("apq_size", &Player::apq_size)
        .def_readwrite("video_filter", &Player::video_filter)
        .def_readwrite("audio_filter", &Player::audio_filter)
        .def_readwrite("keyframe_cache_size", &Player::keyframe_cache_size)
        .def_readwrite("post_encode", &Player::post_encode)
        .def_readwrite("hw_encoding", &Player::hw_encoding)
        .def_readwrite("hw_device_type", &Player::hw_device_type);
    py::class_<Reader>(m, "Reader")
        .def(py::init<const char*>())
        .def("start_time", &Reader::start_time)
        .def("duration", &Reader::duration)
        .def("bit_rate", &Reader::bit_rate)
        .def("has_video", &Reader::has_video)
        .def("width", &Reader::width)
        .def("height", &Reader::height)
        .def("frame_rate", &Reader::frame_rate)
        .def("pix_fmt", &Reader::pix_fmt)
        .def("str_pix_fmt", &Reader::str_pix_fmt)
        .def("video_codec", &Reader::video_codec)
        .def("str_video_codec", &Reader::str_video_codec)
        .def("video_bit_rate", &Reader::video_bit_rate)
        .def("video_time_base", &Reader::video_time_base)
        .def("has_audio", &Reader::has_audio)
        .def("channels", &Reader::channels)
        .def("sample_rate", &Reader::sample_rate)
        .def("frame_size", &Reader::frame_size)
        .def("channel_layout", &Reader::channel_layout)
        .def("str_channel_layout", &Reader::str_channel_layout)
        .def("sample_format", &Reader::sample_format)
        .def("str_sample_format", &Reader::str_sample_format)
        .def("audio_codec", &Reader::audio_codec)
        .def("str_audio_codec", &Reader::str_audio_codec)
        .def("audio_bit_rate", &Reader::audio_bit_rate)
        .def("audio_time_base", &Reader::audio_time_base)
        .def("request_seek", &Reader::request_seek)
        .def("start_from", &Reader::start_from)
        .def("end_at", &Reader::end_at)
        //.def_readwrite("pipe_out", &Reader::pipe_out)
        .def_readwrite("pipe_out_filename", &Reader::pipe_out_filename)
        //.def_readwrite("pipe_out_dir", &Reader::pipe_out_dir)
        .def_readwrite("vpq_max_size", &Reader::vpq_max_size)
        .def_readwrite("apq_max_size", &Reader::apq_max_size)
        .def_readwrite("show_video_pkts", &Reader::show_video_pkts)
        .def_readwrite("show_audio_pkts", &Reader::show_audio_pkts);
    py::class_<Frame>(m, "Frame", py::buffer_protocol())
        .def(py::init<>())
        .def(py::init<const Frame&>())
        .def("isValid", &Frame::isValid)
        .def("invalidate", &Frame::invalidate)
        .def("width", &Frame::width)
        .def("height", &Frame::height)
        .def("stride", &Frame::stride)
        .def_readwrite("m_rts", &Frame::m_rts)
        .def_buffer([](Frame &m) -> py::buffer_info {
            return py::buffer_info(
                m.data(),
                sizeof(uint8_t),
                py::format_descriptor<uint8_t>::format(),
                3,
                { m.height(), m.width(), 3},
                { sizeof(uint8_t) * m.stride(), sizeof(uint8_t) * 3, sizeof(uint8_t) }
            );
        });
    py::class_<Decoder>(m, "Decoder")
        .def(py::init<Reader*, AVMediaType>())
        .def(py::init<Reader*, AVMediaType, AVHWDeviceType>())
        .def("sample_rate", &Decoder::sample_rate)
        .def("channels", &Decoder::channels)
        .def("frame_size", &Decoder::frame_size)
        .def("channel_layout", &Decoder::channel_layout)
        .def("sample_format", &Decoder::sample_format)
        .def("bit_rate", &Decoder::bit_rate)
        .def("width", &Decoder::width)
        .def("height", &Decoder::height)
        .def("pix_fmt", &Decoder::pix_fmt)
        .def("nb_frames", &Decoder::nb_frames)
        .def("duration", &Decoder::duration)
        .def("time_base", &Decoder::time_base)
        .def_readwrite("show_frames", &Decoder::show_frames);
    py::class_<Filter>(m, "Filter")
        .def(py::init<Decoder&, const char*>())
        .def("width", &Filter::width)
        .def("height", &Filter::height)
        .def("pix_fmt", &Filter::pix_fmt)
        .def("time_base", &Filter::time_base)
        .def("frame_rate", &Filter::frame_rate)
        .def("sample_rate", &Filter::sample_rate)
        .def("channels", &Filter::channels)
        .def("channel_layout", &Filter::channel_layout)
        .def("sample_format", &Filter::sample_format)
        .def("frame_size", &Filter::frame_size)
        .def_readwrite("show_frames", &Filter::show_frames);
    py::class_<Display>(m, "Display")
        .def(py::init<Reader*, void*>())
        .def("initVideo", &Display::initVideo)
//        .def("pin_osd", &Display::pin_osd)
//        .def("enable_status", &Display::enable_status)
        .def_readwrite("hWnd", &Display::hWnd)
        .def_readwrite("audio_playback_format", &Display::sdl_sample_format)
//        .def_readwrite("disable_audio", &Display::disable_audio)
//        .def_readwrite("osd_enabled", &Display::osd_enabled)
//        .def_readwrite("jpg_enabled", &Display::jpg_enabled)
//        .def_readwrite("ignore_video_pts", &Display::ignore_video_pts)
//        .def_readwrite("recent_q_size", &Display::recent_q_size)
//        .def_readwrite("prepend_recent_write", &Display::prepend_recent_write)
//        .def_readwrite("font_file", &Display::font_file)
        .def_readwrite("start_time", &Display::start_time)
        .def_readwrite("duration", &Display::duration)
        .def_readwrite("win_width", &Display::win_width)
        .def_readwrite("win_height", &Display::win_height)
        .def_readwrite("pix_fmt", &Display::pix_fmt)
        .def_readwrite("fullscreen", &Display::fullscreen);
        //.def_readwrite("renderCallback", &Display::renderCallback)
        //.def_readwrite("progressCallback", &Display::progressCallback)
        //.def_readwrite("pythonCallback", &Display::pythonCallback);
/*
    py::class_<Pipe>(m, "Pipe")
        .def(py::init<Reader&>());
    py::class_<Writer>(m, "Writer")
        .def(py::init<const std::string&>())
        .def_readwrite("enabled", &Writer::enabled)
        .def_readwrite("write_dir", &Writer::write_dir)
        .def_readwrite("filename", &Writer::filename)
        .def_readwrite("show_video_pkts", &Writer::show_video_pkts)
        .def_readwrite("show_audio_pkts", &Writer::show_audio_pkts);
    py::class_<Encoder>(m, "Encoder")
        .def(py::init<Writer&, AVMediaType>())
        .def("video_in", &Encoder::video_in)
        .def("audio_in", &Encoder::audio_in)
        .def("video_out", &Encoder::video_out)
        .def("audio_out", &Encoder::audio_out)
        .def("set_video_in", &Encoder::set_video_in)
        .def("set_audio_in", &Encoder::set_audio_in)
        .def("set_video_out", &Encoder::set_video_out)
        .def("set_audio_out", &Encoder::set_audio_out)
        .def("set_channel_layout_mono", &Encoder::set_channel_layout_mono)
        .def("set_channel_layout_stereo", &Encoder::set_channel_layout_stereo)
        .def_readwrite("pix_fmt", &Encoder::pix_fmt)
        .def_readwrite("width", &Encoder::width)
        .def_readwrite("height", &Encoder::height)
        .def_readwrite("video_bit_rate", &Encoder::video_bit_rate)
        .def_readwrite("frame_rate", &Encoder::frame_rate)
        .def_readwrite("gop_size", &Encoder::gop_size)
        .def_readwrite("video_time_base", &Encoder::video_time_base)
        .def_readwrite("profile", &Encoder::profile)
        .def_readwrite("hw_video_codec_name", &Encoder::hw_video_codec_name)
        .def_readwrite("hw_device_type", &Encoder::hw_device_type)
        .def_readwrite("hw_pix_fmt", &Encoder::hw_pix_fmt)
        .def_readwrite("sw_pix_fmt", &Encoder::sw_pix_fmt)
        .def_readwrite("sample_fmt", &Encoder::sample_fmt)
        .def_readwrite("channel_layout", &Encoder::channel_layout)
        .def_readwrite("audio_bit_rate", &Encoder::audio_bit_rate)
        .def_readwrite("sample_rate", &Encoder::sample_rate)
        .def_readwrite("nb_samples", &Encoder::nb_samples)
        .def_readwrite("channels", &Encoder::channels)
        .def_readwrite("audio_time_base", &Encoder::audio_time_base)
        .def_readwrite("show_frames", &Encoder::show_frames)
        .def_readwrite("frame_q_max_size", &Encoder::frame_q_max_size)
        .def_readwrite("pkt_q_name", &Encoder::pkt_q_name)
        .def_readwrite("frame_q_name", &Encoder::frame_q_name);
*/
    py::class_<AVRational>(m, "AVRational")
        .def(py::init<>())
        .def_readwrite("num", &AVRational::num)
        .def_readwrite("den", &AVRational::den);
    py::enum_<AVMediaType>(m, "AVMediaType")
        .value("AVMEDIA_TYPE_UNKNOWN", AVMediaType::AVMEDIA_TYPE_UNKNOWN)
        .value("AVMEDIA_TYPE_VIDEO", AVMediaType::AVMEDIA_TYPE_VIDEO)
        .value("AVMEDIA_TYPE_AUDIO", AVMediaType::AVMEDIA_TYPE_AUDIO)
        .export_values();
    py::enum_<AVPixelFormat>(m, "AVPixelFormat")
        .value("AV_PIX_FMT_NONE", AVPixelFormat::AV_PIX_FMT_NONE)
        .value("AV_PIX_FMT_YUV420P", AVPixelFormat::AV_PIX_FMT_YUV420P)
        .value("AV_PIX_FMT_RGB24", AVPixelFormat::AV_PIX_FMT_RGB24)
        .value("AV_PIX_FMT_BGR24", AVPixelFormat::AV_PIX_FMT_BGR24)
        .value("AV_PIX_FMT_NV12", AVPixelFormat::AV_PIX_FMT_NV12)
        .value("AV_PIX_FMT_NV21", AVPixelFormat::AV_PIX_FMT_NV21)
        .value("AV_PIX_FMT_RGBA", AVPixelFormat::AV_PIX_FMT_RGBA)
        .value("AV_PIX_FMT_BGRA", AVPixelFormat::AV_PIX_FMT_BGRA)
        .value("AV_PIX_FMT_VAAPI", AVPixelFormat::AV_PIX_FMT_VAAPI)
        .value("AV_PIX_FMT_CUDA", AVPixelFormat::AV_PIX_FMT_CUDA)
        .value("AV_PIX_FMT_QSV", AVPixelFormat::AV_PIX_FMT_QSV)
        .value("AV_PIX_FMT_D3D11VA_VLD", AVPixelFormat::AV_PIX_FMT_D3D11VA_VLD)
        .value("AV_PIX_FMT_VDPAU", AVPixelFormat::AV_PIX_FMT_VDPAU)
        .value("AV_PIX_FMT_D3D11", AVPixelFormat::AV_PIX_FMT_D3D11)
        .value("AV_PIX_FMT_OPENCL", AVPixelFormat::AV_PIX_FMT_OPENCL)
        .value("AV_PIX_FMT_GRAY8", AVPixelFormat::AV_PIX_FMT_GRAY8)
        .export_values();
    py::enum_<AVHWDeviceType>(m, "AVHWDeviceType")
        .value("AV_HWDEVICE_TYPE_NONE", AVHWDeviceType::AV_HWDEVICE_TYPE_NONE)
        .value("AV_HWDEVICE_TYPE_VDPAU", AVHWDeviceType::AV_HWDEVICE_TYPE_VDPAU)
        .value("AV_HWDEVICE_TYPE_CUDA", AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA)
        .value("AV_HWDEVICE_TYPE_VAAPI", AVHWDeviceType::AV_HWDEVICE_TYPE_VAAPI)
        .value("AV_HWDEVICE_TYPE_DXVA2", AVHWDeviceType::AV_HWDEVICE_TYPE_DXVA2)
        .value("AV_HWDEVICE_TYPE_QSV", AVHWDeviceType::AV_HWDEVICE_TYPE_QSV)
        .value("AV_HWDEVICE_TYPE_VIDEOTOOLBOX", AVHWDeviceType::AV_HWDEVICE_TYPE_VIDEOTOOLBOX)
        .value("AV_HWDEVICE_TYPE_D3D11VA", AVHWDeviceType::AV_HWDEVICE_TYPE_D3D11VA)
        .value("AV_HWDEVICE_TYPE_DRM", AVHWDeviceType::AV_HWDEVICE_TYPE_DRM)
        .value("AV_HWDEVICE_TYPE_OPENCL", AVHWDeviceType::AV_HWDEVICE_TYPE_OPENCL)
        .value("AV_HWDEVICE_TYPE_MEDIACODEC", AVHWDeviceType::AV_HWDEVICE_TYPE_MEDIACODEC)
        .export_values();
    py::enum_<AVSampleFormat>(m, "AVSampleFormat")
        .value("AV_SAMPLE_FMT_NONE", AVSampleFormat::AV_SAMPLE_FMT_NONE)
        .value("AV_SAMPLE_FMT_U8", AVSampleFormat::AV_SAMPLE_FMT_U8)
        .value("AV_SAMPLE_FMT_S16", AVSampleFormat::AV_SAMPLE_FMT_S16)
        .value("AV_SAMPLE_FMT_S32", AVSampleFormat::AV_SAMPLE_FMT_S32)
        .value("AV_SAMPLE_FMT_FLT", AVSampleFormat::AV_SAMPLE_FMT_FLT)
        .value("AV_SAMPLE_FMT_DBL", AVSampleFormat::AV_SAMPLE_FMT_DBL)
        .value("AV_SAMPLE_FMT_U8P", AVSampleFormat::AV_SAMPLE_FMT_U8P)
        .value("AV_SAMPLE_FMT_S16P", AVSampleFormat::AV_SAMPLE_FMT_S16P)
        .value("AV_SAMPLE_FMT_S32P", AVSampleFormat::AV_SAMPLE_FMT_S32P)
        .value("AV_SAMPLE_FMT_FLTP", AVSampleFormat::AV_SAMPLE_FMT_FLTP)
        .value("AV_SAMPLE_FMT_DBLP", AVSampleFormat::AV_SAMPLE_FMT_DBLP)
        .value("AV_SAMPLE_FMT_S64", AVSampleFormat::AV_SAMPLE_FMT_S64)
        .value("AV_SAMPLE_FMT_S64P", AVSampleFormat::AV_SAMPLE_FMT_S64P)
        .value("AV_SAMPLE_FMT_NB", AVSampleFormat::AV_SAMPLE_FMT_NB)
        .export_values();

    #ifdef VERSION_INFO
        m.attr("__version__") = VERSION_INFO;
    #else
        m.attr("__version__") = "dev-1a";
    #endif

}

}