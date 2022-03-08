// ../fst contains https://github.com/biocommando/fst-extension.git
#include "../fst/src/FstAudioEffect.h"
#include <string>
#include <sstream>
#include "StaticCompressor.h"

constexpr int param_rec_status = 0;
constexpr int param_level = 1;
constexpr int param_silence_lev_threshold = 2;
constexpr int param_silence_time_threshold = 3;
constexpr int param_buffer_peak_threshold = 4;
constexpr int param_use_rms = 5;
constexpr int num_params = 6;

class StaticCompressorPlugin : public FstAudioEffect
{
    float params[num_params];
    StaticCompressor compressor;

public:
    StaticCompressorPlugin()
    {
        flags |= effFlagsProgramChunks;
        numParams = num_params;
        numPrograms = 0;
        uniqueID = 12345;
        params[param_rec_status] = 0;
        params[param_level] = 0.1;
        params[param_silence_lev_threshold] = 0.1;
        params[param_silence_time_threshold] = 0.1;
        sync_params();
    }

    void setParameter(int index, float value)
    {
        if (index >= 0 && index < num_params)
        {
            params[index] = value;
            sync_params();
        }
    }

    void sync_params()
    {
        compressor.gain = powf(10, -30 * (1 - params[param_level]) / 20);
        compressor.silence_level_threshold = powf(10, (-60 * (1 - params[param_silence_lev_threshold])) / 20);
        compressor.silence_time_threshold = params[param_silence_time_threshold] * sampleRate / region_length_samples;
        compressor.buffer_peak_threshold = powf(10, (-60 * (1 - params[param_buffer_peak_threshold])) / 20);
        compressor.rec_use_rms = params[param_use_rms] > 0.5;
        if (params[param_rec_status] <= 0.5)
        {
            compressor.sync();
        }
    }

    float getParameter(int index)
    {
        if (index < 0 || index >= num_params)
            return 0;
        return params[index];
    }

    std::string getParamName(int index)
    {
        if (index == param_rec_status)
            return "Record";
        if (index == param_level)
            return "Level";
        if (index == param_silence_lev_threshold)
            return "Silence lev";
        if (index == param_silence_time_threshold)
            return "Silence time";
        if (index == param_buffer_peak_threshold)
            return "Peak threshold";
        if (index == param_use_rms)
            return "Use RMS";
        return "";
    }

    std::string getParamDisplay(int index)
    {
        if (index == param_rec_status || index == param_use_rms)
            return params[index] > 0.5 ? "On" : "Off";
        if (index == param_level)
            return std::to_string(-30 * (1 - params[index])).substr(0, 4);
        if (index == param_silence_lev_threshold || index == param_buffer_peak_threshold)
            return std::to_string(-60 * (1 - params[index])).substr(0, 4);
        if (index == param_silence_time_threshold)
            return std::to_string(params[index]).substr(0, 4);
        return "";
    }

    std::string getParamLabel(int index)
    {
        if (index == param_level || index == param_silence_lev_threshold || index == param_buffer_peak_threshold)
            return "dB";
        if (index == param_silence_time_threshold)
            return "sec";
        return "";
    }

    void processReplacing(float **indata, float **outdata, int sampleframes)
    {
        auto time_info = reinterpret_cast<VstTimeInfo *>(audioMasterDispatch(audioMasterGetTime, 0, 0, nullptr, 0));
        auto sample_pos = static_cast<int>(time_info->samplePos);
        for (int i = 0; i < sampleframes; i++)
        {
            if (params[param_rec_status] > 0.5)
            {
                compressor.record(indata[0][i], indata[1][i], sample_pos);
                outdata[0][i] = indata[0][i];
                outdata[1][i] = indata[1][i];
            }
            else
            {
                auto left = indata[0][i];
                auto right = indata[1][i];
                compressor.process(&left, &right, sample_pos);
                outdata[0][i] = left;
                outdata[1][i] = right;
            }
            sample_pos++;
        }
    }

    int getChunk(void **ptr) override
    {
        FST_DEBUG_LOG("StaticCompressor", "getChunk custom implementation");
        std::string s;
        s = s + std::to_string(num_params) + " ";
        for (int i = 0; i < num_params; i++)
        {
            s = s + std::to_string(params[i]) + " ";
        }
        s = s + std::to_string(compressor.get_num_peaks()) + " ";
        for (int i = 0; i < compressor.get_num_peaks(); i++)
        {
            const auto peak = compressor.get_peak(i);
            if (peak < 1e-10)
                s += "0 ";
            else
                s = s + std::to_string(peak) + " ";
        }
        serialized = s;
        *ptr = (void *)serialized.data();
        FST_DEBUG_LOG("StaticCompressor", "get chunk=" << serialized);
        return serialized.size();
    }

    void setChunk(void *ptr, int size) override
    {
        std::string save_data((char *)ptr, size);
        std::stringstream ss;
        ss << save_data;
        int number_of_params = 0;
        ss >> number_of_params;
        for (int i = 0; i < num_params && i < number_of_params; i++)
        {
            ss >> params[i];
        }
        int number_of_samples = 0;
        ss >> number_of_samples;
        if (number_of_samples < num_samples)
        {
            for (int i = 0; i < number_of_samples; i++)
            {
                float f = 0;
                ss >> f;
                compressor.set_peak(i, f);
            }
        }
        sync_params();
    }
};

FstAudioEffect *createFstInstance()
{
    return new StaticCompressorPlugin();
}