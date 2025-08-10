#pragma once

#include <string>
#include <type_traits>
#include <stdexcept>
#include <iostream>
#include <cmath>

namespace ParticleZoo
{

    template<typename T>
    class Progress {
        static_assert(
            std::is_arithmetic<T>::value &&
            !std::is_same<typename std::remove_cv<T>::type, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char>::value &&
            !std::is_same<typename std::remove_cv<T>::type, signed char>::value &&
            !std::is_same<typename std::remove_cv<T>::type, unsigned char>::value,
            "Progress requires a numeric (integral/floating) type"
        );

        public:
            explicit Progress(T totalProgress);

            // Start a new progress operation
            void Start(const std::string& header);

            // Update the progress with a new message
            void Update(T currentProgress);
            void Update(const std::string& message);
            void Update(T currentProgress, const std::string& message, bool forceUpdate = true);

            // Complete the progress operation
            void Complete();
            void Complete(const std::string& finalMessage);

        private:
            bool is_active_;               // Indicates if progress is active
            std::string start_message_;    // Initial message when progress starts
            std::string current_message_;  // Current progress message
            int last_block_count_;         // Last block count for progress bar
            std::size_t last_render_len_;  // Last rendered length of the progress bar

            T current_progress_; // Current progress value
            T total_progress_;   // Total progress value

            void UpdateProgressBar(int percentageProgress, int progressBarBlocks, const std::string& message);
    };

    // Implementation of Progress methods

    template<typename T>
    inline Progress<T>::Progress(T totalProgress)
    : is_active_(false), start_message_(), current_message_(), last_block_count_(0), last_render_len_(0), current_progress_(0), total_progress_(totalProgress)
    {
        if (total_progress_ <= 0) {
            throw std::invalid_argument("Total progress must be greater than zero.");
        }
        if constexpr (std::is_floating_point<T>::value) {
            if (!std::isfinite(total_progress_)) {
                throw std::invalid_argument("Total progress must be a finite number.");
            }
        }
    }

    template<typename T>
    inline void Progress<T>::Start(const std::string& header) 
    {
        if (is_active_) {
            throw std::runtime_error("Progress is already active.");
        }
        is_active_ = true;
        start_message_ = header;
        last_block_count_ = 0;
        last_render_len_ = 0;
        current_progress_ = 0;
        current_message_ = "";
        UpdateProgressBar(0, 0, current_message_);
    }

    template<typename T>
    inline void Progress<T>::Update(T currentProgress)
    {
        Update(currentProgress, current_message_, false);
    }

    template<typename T>
    inline void Progress<T>::Update(const std::string& message)
    {
        Update(current_progress_, message, true);
    }

    template<typename T>
    inline void Progress<T>::Update(T currentProgress, const std::string& message, bool forceUpdate)
    {
        if (!is_active_) {
            throw std::runtime_error("Progress is not active.");
        }

        if constexpr (std::is_signed<T>::value) {
            if (currentProgress < 0) {
                currentProgress = 0;
            }
        }

        if (currentProgress > total_progress_) {
            currentProgress = total_progress_;
        }

        if constexpr (std::is_floating_point<T>::value) {
            if (!std::isfinite(currentProgress)) {
                throw std::invalid_argument("Current progress must be a finite number.");
            }
        }

        current_progress_ = currentProgress;

        const long double progressFraction = static_cast<long double>(current_progress_) / static_cast<long double>(total_progress_);
        int percentageProgress = static_cast<int>(std::lround(progressFraction * 100.0L));
        int progressBarBlocks = percentageProgress / 5; // 20 steps for 100%

        // Early-exit rules:
        // - Always skip when blocks unchanged and message unchanged (even if forceUpdate).
        // - When not forcing, skip whenever blocks are unchanged (avoid string comparison for hot loops).
        if (progressBarBlocks == last_block_count_) {
            if (!forceUpdate) {
                return;
            }
            if (current_message_ == message) {
                return;
            }
        }

        current_message_ = message;

        UpdateProgressBar(percentageProgress, progressBarBlocks, current_message_);

        last_block_count_ = progressBarBlocks; // Update last block count to the current state
    }

    template<typename T>
    inline void Progress<T>::Complete()
    {
        Complete(current_message_);
    }

    template<typename T>
    inline void Progress<T>::Complete(const std::string& finalMessage)
    {
        if (!is_active_) {
            return; // No active progress to complete
        }
        is_active_ = false;
        current_progress_ = total_progress_;
        int percentageProgress = 100;
        int progressBarBlocks = percentageProgress / 5; // 20 steps for 100%
        UpdateProgressBar(percentageProgress, progressBarBlocks, finalMessage);
        std::cout << std::endl; // Move to the next line after completion
        last_block_count_ = progressBarBlocks; // Update last block count to the final state
    }

    template<typename T>
    inline void Progress<T>::UpdateProgressBar(int percentageProgress, int progressBarBlocks, const std::string& message)
    {
        if (percentageProgress < 0 || percentageProgress > 100) {
            throw std::out_of_range("Percentage progress must be between 0 and 100.");
        }

        int digits = (percentageProgress >= 100) ? 3 : (percentageProgress >= 10) ? 2 : 1;
        int spaces_after = 4 - digits; // 4 spaces for percentage display

        // Compute length of the content to clear trailing characters from previous longer line
        std::size_t content_len =
            start_message_.size() + 2 + 20 + 2 + static_cast<std::size_t>(digits) + 1 +
            static_cast<std::size_t>(spaces_after) + message.size();
        std::size_t pad = (last_render_len_ > content_len) ? (last_render_len_ - content_len) : 0;

        std::cout << "\r" << start_message_ << " [";
        std::cout << std::string(progressBarBlocks, '#') << std::string(20 - progressBarBlocks, '-') << "] "
                  << percentageProgress << "%" << std::string(spaces_after, ' ') << message << std::string(pad, ' ') << std::flush;

        last_render_len_ = content_len + pad;
    }

}