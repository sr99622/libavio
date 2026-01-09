/********************************************************************
* libavio/include/Exception.hpp
*
* Copyright (c) 2022, 2025  Stephen Rhodes
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

#pragma warning(disable: 26812)  // unscoped enum 

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <iostream>
#include <sstream>

extern "C" {
#include <libavutil/avutil.h>
}

namespace avio {

enum CmdTag {
    NO_TAG,
    AO2,
    AOI,
    ACI,
    AFSI,
    APTC,
    APFC,
    AWH,
    AWT,
    AO,
    AC,
    AF,
    ACP,
    AAOC2,
    AFMW,
    AFGB,
    AHCC,
    AFBS,
    AWF,
    ASP,
    ASF,
    AEV2,
    ARF,
    ADV2,
    ARP,
    AIWF,
    AFE,
    AFD,
    AAC3,
    AFA,
    AAC,
    AFC,
    ABR,
    AGF,
    AGA,
    AGC,
    AL,
    AGPP,
    AGCF,
    AHCA,
    AHCI,
    AHGB,
    AFEBN,
    AICTB,
    AGPFN,
    ABAFF,
    APFDG,
    AHFTBN,
    AOSI,
    AOSIL,
    AFDBN,
    ACLFM,
    ACLD,
    AGHC,
    AHTD,
    ANS,
    SGC,
    AFIF,
    APA,
    ADC,
    AIA,
    AFR,
    AFCP,
    APR,
    APC,
    APCP,
    AM,
    SASO,
    SA,
    SI,
    SC,
    SS
};

class ExceptionChecker {

public:
    void ck(int ret) {
        if (ret < 0) throw std::runtime_error("an AV exception has occurred");
    }

    void ck(int ret, CmdTag cmd_tag) {
        if (ret < 0) {
            char av_str[256];
            av_strerror(ret, av_str, 256);
            std::stringstream str;
            str << tag(cmd_tag) << " has failed with error (" << ret << "): " << av_str;
            throw std::runtime_error(str.str());
        }
    }

    void eof(int ret, CmdTag cmd_tag) {
        if (ret < 0) {
            std::stringstream str;
            if (ret == AVERROR_EOF) {
                str << "EOF";
            }
            else {
                char av_str[256];
                av_strerror(ret, av_str, 256);
                str << tag(cmd_tag) << " has failed with error (" << ret << "): " << av_str;
            }
            throw std::runtime_error(str.str());
        }
    }

    void ck(int ret, const std::string& msg) {
        if (ret < 0) {
            char av_str[256];
            av_strerror(ret, av_str, 256);
            std::stringstream str;
            str << msg << " : " << av_str;
            throw std::runtime_error(str.str());
        }
    }

    void ck(void* arg, CmdTag cmd_tag = CmdTag::NO_TAG) {
        if (arg == NULL) {
            if (cmd_tag == CmdTag::NO_TAG) {
                throw std::runtime_error("a NULL exception has occurred");
            }
            else {
                std::stringstream str;
                str << tag(cmd_tag) << " has failed with NULL value";
                throw std::runtime_error(str.str());
            }
        }
    }

    void ck(void* arg, std::string msg1, std::string msg2) {
        if (!arg) {
            std::stringstream str;
            str << msg1 << " : " << msg2;
            throw std::runtime_error(str.str());
        }
    }

    const char* tag(CmdTag cmd_tag) {
        switch (cmd_tag) {
        case CmdTag::AO2:
            return "avcodec_open2";
        case CmdTag::AOI:
            return "avformat_open_input";
        case CmdTag::ACI:
            return "avformat_close_input";
        case CmdTag::AFSI:
            return "avformat_find_stream_info";
        case CmdTag::AFBS:
            return "av_find_best_stream";
        case CmdTag::APTC:
            return "avcodec_parameters_to_context";
        case CmdTag::APFC:
            return "avcodec_parameters_from_context";
        case CmdTag::AWH:
            return "avformat_write_header";
        case CmdTag::AWT:
            return "av_write_trailer";
        case CmdTag::AO:
            return "avio_open";
        case CmdTag::AC:
            return "avio_close";
        case CmdTag::AF:
            return "avio_flush";
        case CmdTag::ACP:
            return "avio_closep";
        case CmdTag::AAOC2:
            return "avformat_alloc_output_context2";
        case CmdTag::AFMW:
            return "av_frame_make_writable";
        case CmdTag::AFGB:
            return "av_frame_get_buffer";
        case CmdTag::AHCC:
            return "av_hwdevice_ctx_create";
        case CmdTag::AWF:
            return "av_write_frame";
        case CmdTag::ASP:
            return "avcodec_send_packet";
        case CmdTag::ASF:
            return "av_seek_frame";
        case CmdTag::AEV2:
            return "avcodec_encode_video2";
        case CmdTag::ARF:
            return "av_read_frame";
        case CmdTag::ADV2:
            return "av_decode_video2";
        case CmdTag::ARP:
            return "avcodec_recieve_packet";
        case CmdTag::AIWF:
            return "av_interleaved_write_frame";
        case CmdTag::AFE:
            return "avcodec_find_encoder";
        case CmdTag::AFD:
            return "avcodec_find_decoder";
        case CmdTag::AAC3:
            return "avcodec_alloc_context3";
        case CmdTag::AFA:
            return "av_frame_alloc";
        case CmdTag::AAC:
            return "avformat_alloc_context";
        case CmdTag::AFC:
            return "av_frame_clone";
        case CmdTag::ABR:
            return "av_buffer_ref";
        case CmdTag::AGF:
            return "av_guess_format";
        case CmdTag::AGA:
            return "avfilter_graph_alloc";
        case CmdTag::AGC:
            return "avfilter_graph_config";
        case CmdTag::AL:
            return "avfilter_link";
        case CmdTag::AGPP:
            return "avfilter_graph_parse_ptr";
        case CmdTag::AGCF:
            return "avfilter_graph_create_filter";
        case CmdTag::AHCA:
            return "avcodec_find_encoder_by_name";
        case CmdTag::AHCI:
            return "av_hwframe_ctx_init";
        case CmdTag::AHGB:
            return "av_hwframe_get_buffer";
        case CmdTag::AFEBN:
            return "avcodec_find_encoder_by_name";
        case CmdTag::AICTB:
            return "av_image_copy_to_buffer";
        case CmdTag::APFDG:
            return "av_pix_fmt_desc_get";
        case CmdTag::AGPFN:
            return "av_get_pix_fmt_name";
        case CmdTag::ABAFF:
            return "av_buffersrc_add_frame_flags";
        case CmdTag::AHFTBN:
            return "av_hwdevice_find_type_by_name";
        case CmdTag::AOSI:
            return "av_opt_set_init";
        case CmdTag::AOSIL:
            return "av_opt_set_int_list";
        case CmdTag::AFDBN:
            return "avcodec_find_decoder_by_name";
        case CmdTag::ACLFM:
            return "av_channel_layout_from_mask";
        case CmdTag::ACLD:
            return "av_channel_layout_describe";
        case CmdTag::AGHC:
            return "avcodec_get_hw_config";
        case CmdTag::AHTD:
            return "av_hwframe_transfer_data";
        case CmdTag::ANS:
            return "avformat_new_stream";
        case CmdTag::AFR:
            return "av_frame_ref";
        case CmdTag::AFCP:
            return "av_frame_copy_props";
        case CmdTag::APR:
            return "av_packet_ref";
        case CmdTag::APC:
            return "av_packet_clone";
        case CmdTag::APCP:
            return "av_packet_copy_props";
        case CmdTag::SGC:
            return "sws_getContext";
        case CmdTag::AFIF:
            return "av_find_input_format";
        case CmdTag::APA:
            return "av_packet_alloc";
        case CmdTag::ADC:
            return "av_dict_copy";
        case CmdTag::AIA:
            return "av_image_alloc";
        case CmdTag::AM:
            return "av_malloc";
        case CmdTag::SASO:
            return "swr_alloc_set_opts";
        case CmdTag::SA:
            return "swr_alloc";
        case CmdTag::SI:
            return "swr_init";
        case CmdTag::SC:
            return "swr_convert";
        case CmdTag::SS:
            return "sws_scale";
        default:
            return "";
        }
    }
};

}


#endif // EXCEPTION_H
