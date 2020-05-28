// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "Open3D/IO/ClassIO/PointCloudIO.h"

#include <iostream>
#include <unordered_map>

#include "Open3D/Utility/Console.h"
#include "Open3D/Utility/FileSystem.h"
#include "Open3D/Utility/Helper.h"
#include "Open3D/Utility/ProgressReporters.h"

namespace open3d {

namespace {
using namespace io;

static const std::unordered_map<
        std::string,
        std::function<bool(const std::string &,
                           geometry::PointCloud &,
                           const ReadPointCloudParams &)>>
        file_extension_to_pointcloud_read_function{
                {"xyz", ReadPointCloudFromXYZ},
                {"xyzn", ReadPointCloudFromXYZN},
                {"xyzrgb", ReadPointCloudFromXYZRGB},
                {"ply", ReadPointCloudFromPLY},
                {"pcd", ReadPointCloudFromPCD},
                {"pts", ReadPointCloudFromPTS},
        };

static const std::unordered_map<
        std::string,
        std::function<bool(const std::string &,
                           const geometry::PointCloud &,
                           const WritePointCloudParams &)>>
        file_extension_to_pointcloud_write_function{
                {"xyz", WritePointCloudToXYZ},
                {"xyzn", WritePointCloudToXYZN},
                {"xyzrgb", WritePointCloudToXYZRGB},
                {"ply", WritePointCloudToPLY},
                {"pcd", WritePointCloudToPCD},
                {"pts", WritePointCloudToPTS},
        };
}  // unnamed namespace

namespace io {

std::shared_ptr<geometry::PointCloud> CreatePointCloudFromFile(
        const std::string &filename,
        const std::string &format,
        bool print_progress) {
    auto pointcloud = std::make_shared<geometry::PointCloud>();
    ReadPointCloud(filename, *pointcloud, format, print_progress);
    return pointcloud;
}

bool ReadPointCloudP(const std::string &filename,
                     geometry::PointCloud &pointcloud,
                     const ReadPointCloudParams &params) {
    std::string format = params.format;
    if (format == "auto") {
        format = utility::filesystem::GetFileExtensionInLowerCase(filename);
    }

    utility::LogDebug("Format {} File {}", params.format, filename);

    auto map_itr = file_extension_to_pointcloud_read_function.find(format);
    if (map_itr == file_extension_to_pointcloud_read_function.end()) {
        utility::LogWarning(
                "Read geometry::PointCloud failed: unknown file extension for "
                "{} (format: {}).",
                filename, params.format);
        return false;
    }
    bool success = map_itr->second(filename, pointcloud, params);
    utility::LogDebug("Read geometry::PointCloud: {:d} vertices.",
                      (int)pointcloud.points_.size());
    if (params.remove_nan_points || params.remove_infinite_points) {
        pointcloud.RemoveNonFinitePoints(params.remove_nan_points,
                                         params.remove_infinite_points);
    }
    return success;
}
bool ReadPointCloud(const std::string &filename,
                    geometry::PointCloud &pointcloud,
                    const std::string &file_format,
                    bool remove_nan_points,
                    bool remove_infinite_points,
                    bool print_progress) {
    std::string format = file_format;
    if (format == "auto") {
        format = utility::filesystem::GetFileExtensionInLowerCase(filename);
    }

    ReadPointCloudParams p;
    p.format = format;
    p.remove_nan_points = remove_nan_points;
    p.remove_infinite_points = remove_infinite_points;
    utility::ConsoleProgressUpdater progress_updater(
            std::string("Reading ") + utility::ToUpper(format) +
                    " file: " + filename,
            print_progress);
    p.update_progress = progress_updater;
    return ReadPointCloudP(filename, pointcloud, p);
}

bool WritePointCloudP(const std::string &filename,
                      const geometry::PointCloud &pointcloud,
                      const WritePointCloudParams &params) {
    std::string format =
            utility::filesystem::GetFileExtensionInLowerCase(filename);
    auto map_itr = file_extension_to_pointcloud_write_function.find(format);
    if (map_itr == file_extension_to_pointcloud_write_function.end()) {
        utility::LogWarning(
                "Write geometry::PointCloud failed: unknown file extension {} "
                "for file {}.",
                format, filename);
        return false;
    }

    bool success = map_itr->second(filename, pointcloud, params);
    utility::LogDebug("Write geometry::PointCloud: {:d} vertices.",
                      (int)pointcloud.points_.size());
    return success;
}
bool WritePointCloud(const std::string &filename,
                     const geometry::PointCloud &pointcloud,
                     bool write_ascii /* = false*/,
                     bool compressed /* = false*/,
                     bool print_progress) {
    WritePointCloudParams p;
    p.write_ascii = WritePointCloudParams::IsAscii(write_ascii);
    p.compressed = WritePointCloudParams::Compressed(compressed);
    std::string format =
            utility::filesystem::GetFileExtensionInLowerCase(filename);
    utility::ConsoleProgressUpdater progress_updater(
            std::string("Reading ") + utility::ToUpper(format) +
                    " file: " + filename,
            print_progress);
    p.update_progress = progress_updater;
    return WritePointCloudP(filename, pointcloud, p);
}

}  // namespace io
}  // namespace open3d
