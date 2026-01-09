/********************************************************************
* libavio/src/avio.cpp
*
* Copyright (c) 2023, 2025  Stephen Rhodes
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
#include "Player.hpp"
#include "Reader.hpp"
#include "Frame.hpp"
#include "Audio.hpp"

namespace py = pybind11;

namespace avio {

PYBIND11_MODULE(avio, m)
{
    m.doc() = "pybind11 av plugin";
    py::class_<Player>(m, "Player")
        .def(py::init<const std::string&>())
        .def("__eq__", &Player::operator==)
        .def("__str__", &Player::toString)
        .def("play", &Player::play)
        .def("start", &Player::start)
        .def("seek", &Player::seek)
        .def("width", &Player::width)
        .def("height", &Player::height)
        .def("isPaused", &Player::isPaused)
        .def("isRecording", &Player::isRecording)
        .def("isMuted", &Player::isMuted)
        .def("isCameraStream", &Player::isCameraStream)
        .def("setVolume", &Player::setVolume)
        .def("getVolume", &Player::getVolume)
        .def("setMute", &Player::setMute)
        .def("hasAudio", &Player::hasAudio)
        .def("hasVideo", &Player::hasVideo)
        .def("setMetaData", &Player::setMetaData)
        .def("togglePaused", &Player::togglePaused)
        .def("toggleRecording", &Player::toggleRecording)
        .def("startFileBreak", &Player::startFileBreak)
        .def("getAudioCodec", &Player::getAudioCodec)
        .def("clearBuffer", &Player::clearBuffer)
        .def("getStreamInfo", &Player::getStreamInfo)
        .def("getFFMPEGVersions", &Player::getFFMPEGVersions)
        .def("getAudioDrivers", &Player::getAudioDrivers)
        .def("getHardwareDecoders", &Player::getHardwareDecoders)
        .def("duration", &Player::duration)
        .def("terminate", &Player::terminate)
        .def_readwrite("uri", &Player::uri)
        .def_readwrite("request_reconnect", &Player::request_reconnect)
        .def_readwrite("live_stream", &Player::live_stream)
        .def_readwrite("headless", &Player::headless)
        .def_readwrite("disable_video", &Player::disable_video)
        .def_readwrite("disable_audio", &Player::disable_audio)
        .def_readwrite("hidden", &Player::hidden)
        .def_readwrite("progressCallback", &Player::progressCallback)
        .def_readwrite("renderCallback", &Player::renderCallback)
        .def_readwrite("pyAudioCallback", &Player::pyAudioCallback)
        .def_readwrite("infoCallback", &Player::infoCallback)
        .def_readwrite("errorCallback", &Player::errorCallback)
        .def_readwrite("mediaPlayingStarted", &Player::mediaPlayingStarted)
        .def_readwrite("mediaPlayingStopped", &Player::mediaPlayingStopped)
        .def_readwrite("packetDrop", &Player::packetDrop)
        .def_readwrite("str_video_filter", &Player::str_video_filter)
        .def_readwrite("str_audio_filter", &Player::str_audio_filter)
        .def_readwrite("audio_driver_index", &Player::audio_driver_index)
        .def_readwrite("str_hw_device_type", &Player::str_hw_device_type)
        .def_readwrite("onvif_frame_rate", &Player::onvif_frame_rate)
        .def_readwrite("buffer_size_in_seconds", &Player::buffer_size_in_seconds)
        .def_readwrite("file_start_from_seek", &Player::file_start_from_seek);

    py::class_<Reader>(m, "Reader")
        .def(py::init<const std::string&>())
        .def("start_time", &Reader::start_time)
        .def("duration", &Reader::duration)
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
        .def("str_channel_layout", &Reader::str_channel_layout)
        .def("sample_format", &Reader::sample_format)
        .def("str_sample_format", &Reader::str_sample_format)
        .def("audio_codec", &Reader::audio_codec)
        .def("str_audio_codec", &Reader::str_audio_codec)
        .def("audio_bit_rate", &Reader::audio_bit_rate)
        .def("audio_time_base", &Reader::audio_time_base);

    py::class_<Frame>(m, "Frame", py::buffer_protocol())
        .def(py::init<>())
        .def(py::init<const Frame&>())
        .def("pts", &Frame::pts)
        .def("width", &Frame::width)
        .def("height", &Frame::height)
        .def("stride", &Frame::stride)
        .def("channels", &Frame::channels)
        .def("mb_samples", &Frame::nb_samples)
        .def_buffer([](Frame &m) -> py::buffer_info {
            if (m.height() == 0 && m.width() == 0) {
                return py::buffer_info(
                    m.data(),
                    sizeof(float),
                    py::format_descriptor<float>::format(),
                    1,
                    { m.nb_samples() * m.channels() },
                    { sizeof(float) }
                );
            }
            else {
                py::ssize_t element_size = sizeof(uint8_t);
                std::string fmt_desc =  py::format_descriptor<uint8_t>::format();
                std::vector<py::ssize_t> dims = { m.height(), m.width(), 3};
                py::ssize_t ndim = dims.size();
                std::vector<py::ssize_t> strides = { (long)(sizeof(uint8_t) * m.stride()), (py::ssize_t)(sizeof(uint8_t) * ndim), sizeof(uint8_t) };
                return py::buffer_info(m.data(), element_size, fmt_desc, ndim, dims, strides);
            }
        });

    py::class_<AVRational>(m, "AVRational")
        .def(py::init<>())
        .def_readwrite("num", &AVRational::num)
        .def_readwrite("den", &AVRational::den);

    m.attr("__version__") = "3.2.7";

}

}