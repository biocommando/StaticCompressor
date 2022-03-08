#pragma once

#include <cmath>

constexpr int num_samples = 20000;
constexpr int region_length_samples = 2000;

class StaticCompressor
{
    float peaks[num_samples];
    float gains[num_samples]; // Gain to set the level to 1
    float current_peak = -1;
    int rec_state = 0;
    int max_idx = 0;

public:
    float silence_level_threshold = 1e-5;
    float buffer_peak_threshold = 1e-5;
    int silence_time_threshold = 2;
    float gain = 1;
    bool rec_use_rms = false;

    StaticCompressor()
    {
        for (int i = 0; i < num_samples; i++)
        {
            peaks[i] = 0;
            gains[i] = 0;
        }
    }

    int get_num_peaks()
    {
        return max_idx + 1;
    }

    float get_peak(int i)
    {
        return peaks[i];
    }

    void set_peak(int i, float f)
    {
        peaks[i] = f;
        max_idx = i > max_idx ? i : max_idx;
    }

    void record(float left, float right, int pos)
    {
        if (pos % region_length_samples == 0)
        {
            int arr_pos = pos / region_length_samples;
            if (arr_pos < num_samples && current_peak >= 0)
            {
                peaks[arr_pos] = current_peak;
                if (rec_use_rms)
                {
                    peaks[arr_pos] /= region_length_samples;
                    peaks[arr_pos] = sqrtf(peaks[arr_pos]);
                }
            }
            current_peak = 0;
            rec_state = 1;
        }
        if (!rec_state)
            return;
        const auto abs_left = fabs(left);
        const auto abs_right = fabs(right);
        if (rec_use_rms)
        {
            current_peak += (abs_left * abs_left + abs_right * abs_right) / 2;
        }
        else
        {
            if (abs_left > current_peak)
                current_peak = abs_left;
            if (abs_right > current_peak)
                current_peak = abs_right;
        }
    }

    void process(float *left, float *right, int pos)
    {
        int arr_pos = pos / region_length_samples;
        if (arr_pos >= 0 && arr_pos < num_samples)
        {
            auto f = gains[arr_pos];
            if (arr_pos < num_samples - 1 && fabs(f - gains[arr_pos + 1]) > 1e-6)
            {
                const float interpolation_start_point = arr_pos * region_length_samples;
                const auto diff = (gains[arr_pos + 1] - f);
                f = f + diff * (pos - interpolation_start_point) / region_length_samples;
            }
            f *= gain;
            *left *= f;
            *right *= f;
        }
    }

    void sync()
    {
        rec_state = 0;
        current_peak = -1;
        int silent_buffers = 0;
        int region_start = 0;
        float peak_in_region = 0;
        for (int i = 0; i < num_samples; i++)
        {
            gains[i] = 0;
        }
        for (int i = 0; i < num_samples; i++)
        {
            if (peaks[i] > silence_level_threshold)
            {
                if (region_start == 0)
                {
                    region_start = i - 1;
                    peak_in_region = 0;
                    silent_buffers = 0;
                }
            }
            else if (++silent_buffers == silence_time_threshold)
            {
                auto region_end = i;
                if (peak_in_region > buffer_peak_threshold)
                {
                    for (int i = region_start; i <= region_end; i++)
                    {
                        gains[i] = 1 / peak_in_region;
                    }
                }
                max_idx = region_end > max_idx ? region_end : max_idx;
                region_start = 0;
            }
            if (peaks[i] > peak_in_region)
                peak_in_region = peaks[i];
        }
    }
};