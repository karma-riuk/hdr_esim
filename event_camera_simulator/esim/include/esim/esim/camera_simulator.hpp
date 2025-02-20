#pragma once

#include <deque>
#include <esim/common/types.hpp>
#include <ze/common/time_conversions.hpp>

namespace event_camera_simulator {

    class ImageBuffer {
      public:
        ZE_POINTER_TYPEDEFS(ImageBuffer);

        struct ImageData {
            ImageData(
                Image img, ImageRGB img_rgb, Time stamp, Duration exposure_time
            )
                : image(img),
                  image_rgb(img_rgb),
                  stamp(stamp),
                  exposure_time(exposure_time) {}

            Image image;
            ImageRGB image_rgb;
            Time stamp;
            Duration exposure_time; // timestamp since last image (0 if this is
                                    // the first image)
        };

        using ExposureImage = std::pair<Duration, Image>;

        // Rolling image buffer of mazimum size 'buffer_size_ns'.
        ImageBuffer(Duration buffer_size_ns): buffer_size_ns_(buffer_size_ns) {}

        void addImage(Time t, const Image& img, const ImageRGB& img_rgb);

        std::deque<ImageData> getRawBuffer() const {
            return data_;
        }

        size_t size() const {
            return data_.size();
        }

        Duration getExposureTime() const {
            return buffer_size_ns_;
        }

      private:
        Duration buffer_size_ns_;
        std::deque<ImageData> data_;
    };

    /*
     * The CameraSimulator takes as input a sequence of stamped images,
     * assumed to be sampled at a "sufficiently high" framerate and with
     * floating-point precision, and treats each image as a measure of
     * irradiance.
     * From this, it simulates a real camera, including motion blur.
     *
     * @TODO: simulate a non-linear camera response curve, shot noise, etc.
     */
    class CameraSimulator {
      public:
        CameraSimulator(double exposure_time_ms)
            : exposure_time_(ze::secToNanosec(exposure_time_ms / 1000.0)) {
            buffer_.reset(new ImageBuffer(exposure_time_));
        }

        bool imageCallback(
            const Image& img,
            const ImageRGB& img_rgb,
            Time time,
            const ImagePtr& camera_image
        );

      private:
        ImageBuffer::Ptr buffer_;
        const Duration exposure_time_;
    };

} // namespace event_camera_simulator
