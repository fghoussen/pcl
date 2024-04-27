/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 *
 */

#define MEASURE_FUNCTION_TIME
#include <pcl/common/time.h> //fps calculations
#include <pcl/console/parse.h>
#include <pcl/console/print.h>
#include <pcl/io/openni_grabber.h>
#include <pcl/visualization/image_viewer.h>
#include <pcl/visualization/pcl_visualizer.h>

#include <mutex>

#define SHOW_FPS 1
#if SHOW_FPS
#define FPS_CALC(_WHAT_)                                                               \
  do {                                                                                 \
    static unsigned count = 0;                                                         \
    static double last = pcl::getTime();                                               \
    double now = pcl::getTime();                                                       \
    ++count;                                                                           \
    if (now - last >= 1.0) {                                                           \
      std::cout << "Average framerate(" << (_WHAT_)                                    \
                << "): " << double (count) / double (now - last) << " Hz"              \
                << std::endl;                                                          \
      count = 0;                                                                       \
      last = now;                                                                      \
    }                                                                                  \
  } while (false)
#else
#define FPS_CALC(_WHAT_)                                                               \
  do {                                                                                 \
  } while (false)
#endif

void
printHelp (int, char** argv)
{
  using pcl::console::print_error;
  using pcl::console::print_info;

  print_error ("Syntax is: %s [((<device_id> | <path-to-oni-file>) [-depthmode <mode>] "
               "[-imagemode <mode>] [-xyz] | -l [<device_id>]| -h | --help)]\n",
               argv[0]);
  print_info ("%s -h | --help : shows this help\n", argv[0]);
  print_info ("%s -xyz : use only XYZ values and ignore RGB components (this flag is "
              "required for use with ASUS Xtion Pro) \n",
              argv[0]);
  print_info ("%s -l : list all available devices\n", argv[0]);
  print_info ("%s -l <device-id> :list all available modes for specified device\n",
              argv[0]);
  print_info ("\t\t<device_id> may be \"#1\", \"#2\", ... for the first, second etc "
              "device in the list\n");
#ifndef _WIN32
  print_info ("\t\t                   bus@address for the device connected to a "
              "specific usb-bus / address combination\n");
  print_info ("\t\t                   <serial-number>\n");
#endif
  print_info ("\n\nexamples:\n");
  print_info ("%s \"#1\"\n", argv[0]);
  print_info ("\t\t uses the first device.\n");
  print_info ("%s  \"./temp/test.oni\"\n", argv[0]);
  print_info ("\t\t uses the oni-player device to play back oni file given by path.\n");
  print_info ("%s -l\n", argv[0]);
  print_info ("\t\t list all available devices.\n");
  print_info ("%s -l \"#2\"\n", argv[0]);
  print_info ("\t\t list all available modes for the second device.\n");
#ifndef _WIN32
  print_info ("%s A00361800903049A\n", argv[0]);
  print_info ("\t\t uses the device with the serial number \'A00361800903049A\'.\n");
  print_info ("%s 1@16\n", argv[0]);
  print_info ("\t\t uses the device on address 16 at USB bus 1.\n");
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename PointType>
class OpenNIViewer {
public:
  using Cloud = pcl::PointCloud<PointType>;
  using CloudConstPtr = typename Cloud::ConstPtr;

  OpenNIViewer (pcl::Grabber& grabber)
  : cloud_viewer_ (new pcl::visualization::PCLVisualizer ("PCL OpenNI cloud"))
  , grabber_ (grabber)
  {}

  void
  cloud_callback (const CloudConstPtr& cloud)
  {
    FPS_CALC ("cloud callback");
    std::lock_guard<std::mutex> lock (cloud_mutex_);
    cloud_ = cloud;
  }

  void
  image_callback (const openni_wrapper::Image::Ptr& image)
  {
    FPS_CALC ("image callback");
    std::lock_guard<std::mutex> lock (image_mutex_);
    image_ = image;

    if (image->getEncoding() != openni_wrapper::Image::RGB) {
      if (rgb_data_size_ < image->getWidth() * image->getHeight()) {
        delete[] rgb_data_;
        rgb_data_size_ = image->getWidth() * image->getHeight();
        rgb_data_ = new unsigned char[rgb_data_size_ * 3];
      }
      image_->fillRGB (image_->getWidth(), image_->getHeight(), rgb_data_);
    }
  }

  void
  keyboard_callback (const pcl::visualization::KeyboardEvent& event, void*)
  {
    if (event.getKeyCode())
      std::cout << "the key \'" << event.getKeyCode() << "\' (" << event.getKeyCode()
                << ") was";
    else
      std::cout << "the special key \'" << event.getKeySym() << "\' was";
    if (event.keyDown())
      std::cout << " pressed" << std::endl;
    else
      std::cout << " released" << std::endl;
  }

  void
  mouse_callback (const pcl::visualization::MouseEvent& mouse_event, void*)
  {
    if (mouse_event.getType() == pcl::visualization::MouseEvent::MouseButtonPress &&
        mouse_event.getButton() == pcl::visualization::MouseEvent::LeftButton) {
      std::cout << "left button pressed @ " << mouse_event.getX() << " , "
                << mouse_event.getY() << std::endl;
    }
  }

  /**
   * @brief starts the main loop
   */
  void
  run ()
  {
    cloud_viewer_->registerMouseCallback (&OpenNIViewer::mouse_callback, *this);
    cloud_viewer_->registerKeyboardCallback (&OpenNIViewer::keyboard_callback, *this);
    std::function<void (const CloudConstPtr&)> cloud_cb =
        [this] (const CloudConstPtr& cloud) { cloud_callback (cloud); };
    boost::signals2::connection cloud_connection = grabber_.registerCallback (cloud_cb);

    boost::signals2::connection image_connection;
    if (grabber_.providesCallback<void (const openni_wrapper::Image::Ptr&)>()) {
      image_viewer_.reset (new pcl::visualization::ImageViewer ("PCL OpenNI image"));
      image_viewer_->registerMouseCallback (&OpenNIViewer::mouse_callback, *this);
      image_viewer_->registerKeyboardCallback (&OpenNIViewer::keyboard_callback, *this);
      std::function<void (const openni_wrapper::Image::Ptr&)> image_cb =
          [this] (const openni_wrapper::Image::Ptr& img) { image_callback (img); };
      image_connection = grabber_.registerCallback (image_cb);
    }

    bool image_init = false, cloud_init = false;

    grabber_.start();

    while (!(cloud_viewer_->wasStopped() ||
             (image_viewer_ && image_viewer_->wasStopped()))) {
      openni_wrapper::Image::Ptr image;
      CloudConstPtr cloud;

      cloud_viewer_->spinOnce();

      // See if we can get a cloud
      if (cloud_mutex_.try_lock()) {
        cloud_.swap (cloud);
        cloud_mutex_.unlock();
      }

      if (cloud) {
        FPS_CALC ("drawing cloud");

        if (!cloud_init) {
          cloud_viewer_->setPosition (0, 0);
          cloud_viewer_->setSize (cloud->width, cloud->height);
          cloud_init = true;
        }

        if (!cloud_viewer_->updatePointCloud (cloud, "OpenNICloud")) {
          cloud_viewer_->addPointCloud (cloud, "OpenNICloud");
          cloud_viewer_->resetCameraViewpoint ("OpenNICloud");
        }
      }

      // See if we can get an image
      if (image_mutex_.try_lock()) {
        image_.swap (image);
        image_mutex_.unlock();
      }

      if (image) {
        if (!image_init && cloud && cloud->width != 0) {
          image_viewer_->setPosition (cloud->width, 0);
          image_viewer_->setSize (cloud->width, cloud->height);
          image_init = true;
        }

        if (image->getEncoding() == openni_wrapper::Image::RGB)
          image_viewer_->addRGBImage (image->getMetaData().Data(),
                                      image->getWidth(),
                                      image->getHeight(),
                                      "rgb_image");
        else
          image_viewer_->addRGBImage (
              rgb_data_, image->getWidth(), image->getHeight(), "rgb_image");
        image_viewer_->spinOnce();
      }
    }

    grabber_.stop();

    cloud_connection.disconnect();
    image_connection.disconnect();
    delete[] rgb_data_;
  }

  pcl::visualization::PCLVisualizer::Ptr cloud_viewer_;
  pcl::visualization::ImageViewer::Ptr image_viewer_;

  pcl::Grabber& grabber_;
  std::mutex cloud_mutex_;
  std::mutex image_mutex_;

  CloudConstPtr cloud_;
  openni_wrapper::Image::Ptr image_;
  unsigned char* rgb_data_{nullptr};
  unsigned rgb_data_size_{0};
};

// Create the PCLVisualizer object
pcl::visualization::PCLVisualizer::Ptr cld;
pcl::visualization::ImageViewer::Ptr img;

/* ---[ */
int
main (int argc, char** argv)
{
  std::string device_id;
  pcl::OpenNIGrabber::Mode depth_mode = pcl::OpenNIGrabber::OpenNI_Default_Mode;
  pcl::OpenNIGrabber::Mode image_mode = pcl::OpenNIGrabber::OpenNI_Default_Mode;
  bool xyz = false;

  if (argc >= 2) {
    device_id = argv[1];
    if (device_id == "--help" || device_id == "-h") {
      printHelp (argc, argv);
      return 0;
    }
    if (device_id == "-l") {
      if (argc >= 3) {
        pcl::OpenNIGrabber grabber (argv[2]);
        auto device = grabber.getDevice();
        std::cout << "Supported depth modes for device: " << device->getVendorName()
                  << " , " << device->getProductName() << std::endl;
        std::vector<std::pair<int, XnMapOutputMode>> modes =
            grabber.getAvailableDepthModes();
        for (const auto& mode : modes) {
          std::cout << mode.first << " = " << mode.second.nXRes << " x "
                    << mode.second.nYRes << " @ " << mode.second.nFPS << std::endl;
        }

        if (device->hasImageStream()) {
          std::cout << std::endl
                    << "Supported image modes for device: " << device->getVendorName()
                    << " , " << device->getProductName() << std::endl;
          modes = grabber.getAvailableImageModes();
          for (const auto& mode : modes) {
            std::cout << mode.first << " = " << mode.second.nXRes << " x "
                      << mode.second.nYRes << " @ " << mode.second.nFPS << std::endl;
          }
        }
      }
      else {
        openni_wrapper::OpenNIDriver& driver =
            openni_wrapper::OpenNIDriver::getInstance();
        if (driver.getNumberDevices() > 0) {
          for (unsigned deviceIdx = 0; deviceIdx < driver.getNumberDevices();
               ++deviceIdx) {
            std::cout << "Device: " << deviceIdx + 1
                      << ", vendor: " << driver.getVendorName (deviceIdx)
                      << ", product: " << driver.getProductName (deviceIdx)
                      << ", connected: " << driver.getBus (deviceIdx) << " @ "
                      << driver.getAddress (deviceIdx) << ", serial number: \'"
                      << driver.getSerialNumber (deviceIdx) << "\'" << std::endl;
          }
        }
        else
          std::cout << "No devices connected." << std::endl;

        std::cout << "Virtual Devices available: ONI player" << std::endl;
      }
      return 0;
    }
  }
  else {
    openni_wrapper::OpenNIDriver& driver = openni_wrapper::OpenNIDriver::getInstance();
    if (driver.getNumberDevices() > 0)
      std::cout << "Device Id not set, using first device." << std::endl;
  }

  unsigned mode;
  if (pcl::console::parse (argc, argv, "-depthmode", mode) != -1)
    depth_mode = static_cast<pcl::OpenNIGrabber::Mode> (mode);

  if (pcl::console::parse (argc, argv, "-imagemode", mode) != -1)
    image_mode = static_cast<pcl::OpenNIGrabber::Mode> (mode);

  if (pcl::console::find_argument (argc, argv, "-xyz") != -1)
    xyz = true;

  try {
    pcl::OpenNIGrabber grabber (device_id, depth_mode, image_mode);

    if (xyz ||
        !grabber
             .providesCallback<pcl::OpenNIGrabber::sig_cb_openni_point_cloud_rgb>()) {
      OpenNIViewer<pcl::PointXYZ> openni_viewer (grabber);
      openni_viewer.run();
    }
    else {
      OpenNIViewer<pcl::PointXYZRGBA> openni_viewer (grabber);
      openni_viewer.run();
    }
  } catch (pcl::IOException& e) {
    pcl::console::print_error ("Failed to create a grabber: %s\n", e.what());
    return (1);
  }

  return (0);
}
/* ]--- */
