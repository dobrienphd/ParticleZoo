#pragma once

#include <string>
#include <type_traits>
#include <stdexcept>
#include <iostream>
#include <cmath>

namespace ParticleZoo
{

    /**
     * @brief Template class for displaying console progress bars with customizable numeric types.
     * 
     * The Progress class provides an efficient console progress bar that works with
     * various numeric types (integers and floating-point numbers). It displays progress as both
     * a visual bar and percentage, with support for custom status messages.
     * 
     * Designed to provide a consistent interface across bundled ParticleZoo tools.
     * 
     * Usage pattern:
     * 1. Create Progress instance with total value
     * 2. Call Start() to begin progress tracking
     * 3. Call Update() repeatedly as work progresses
     * 4. Call Complete() when finished
     * 
     * @tparam T The numeric type for progress values (must be arithmetic, not bool/char)
     */
    template<typename T>
    class Progress {
        /**
         * @brief Compile-time type validation for template parameter.
         * 
         * Ensures T is an arithmetic type but excludes bool and char variants
         * which are not suitable for progress tracking.
         */
        static_assert(
            std::is_arithmetic<T>::value &&
            !std::is_same<typename std::remove_cv<T>::type, bool>::value &&
            !std::is_same<typename std::remove_cv<T>::type, char>::value &&
            !std::is_same<typename std::remove_cv<T>::type, signed char>::value &&
            !std::is_same<typename std::remove_cv<T>::type, unsigned char>::value,
            "Progress requires a numeric (integral/floating) type"
        );

        public:
            /**
             * @brief Construct a new Progress object with specified total progress value.
             * 
             * @param totalProgress The maximum progress value (must be > 0 and finite for floating-point)
             * @throws std::invalid_argument if totalProgress <= 0 or is not finite (for floating-point types)
             */
            explicit Progress(T totalProgress);

            /**
             * @brief Start a new progress tracking operation.
             * 
             * Initializes the progress bar display with a header message.
             * The progress bar will show 0% completion and begin accepting updates.
             * Calling Start() is required before any Update() calls.
             * 
             * @param header The header message to display before the progress bar
             * @throws std::runtime_error if progress tracking is already active
             */
            void Start(const std::string& header);

            /**
             * @brief Update the current progress value without changing the message.
             * 
             * Updates the progress bar to reflect the new progress value. The status message
             * remains unchanged from the previous update.
             * 
             * @param currentProgress The new progress value (will be clamped to [0, totalProgress])
             * @throws std::runtime_error if progress tracking is not active
             * @throws std::invalid_argument if currentProgress is not finite (for floating-point types)
             */
            void Update(T currentProgress);
            
            /**
             * @brief Update the status message without changing the progress value.
             * 
             * Updates the status message displayed after the progress bar. The progress
             * value remains unchanged from the previous update.
             * 
             * @param message The new status message to display
             * @throws std::runtime_error if progress tracking is not active
             */
            void Update(const std::string& message);
            
            /**
             * @brief Update both progress value and status message.
             * 
             * Updates both the progress bar and status message. Provides control over
             * whether the update should be forced even if the visual progress hasn't changed.
             * Updates are forced by default but can be disabled for performance in tight loops.
             * 
             * @param currentProgress The new progress value (will be clamped to [0, totalProgress])
             * @param message The new status message to display
             * @param forceUpdate If true, updates display even if progress bar blocks haven't changed (default: true)
             * @throws std::runtime_error if progress tracking is not active
             * @throws std::invalid_argument if currentProgress is not finite (for floating-point types)
             */
            void Update(T currentProgress, const std::string& message, bool forceUpdate = true);

            /**
             * @brief Complete the progress operation with the current message.
             * 
             * Sets progress to 100%, displays the final progress bar, and ends the progress tracking.
             * Moves the cursor to a new line after completion.
             */
            void Complete();
            
            /**
             * @brief Complete the progress operation with a custom final message.
             * 
             * Sets progress to 100%, displays the final progress bar with the specified message,
             * and ends the progress tracking. Moves the cursor to a new line after completion.
             * 
             * @param finalMessage The final status message to display at completion
             */
            void Complete(const std::string& finalMessage);

        private:
            bool is_active_;               ///< Indicates if progress tracking is currently active
            std::string start_message_;    ///< Initial header message displayed when progress starts
            std::string current_message_;  ///< Current status message displayed after the progress bar
            int last_block_count_;         ///< Last number of progress blocks rendered (for optimization)
            std::size_t last_render_len_;  ///< Last total length of rendered progress line (for cleanup)

            T current_progress_;           ///< Current progress value
            T total_progress_;             ///< Maximum progress value (target for 100%)

            /**
             * @brief Update the progress bar.
             * 
             * Renders the progress bar to the console with the specified percentage and message.
             * Handles padding and cleanup of previous longer lines to prevent display artifacts.
             * 
             * @param percentageProgress The progress percentage (0-100)
             * @param progressBarBlocks The number of filled blocks in the 20-character progress bar
             * @param message The status message to display after the progress bar
             * @throws std::out_of_range if percentageProgress is not between 0 and 100
             */
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