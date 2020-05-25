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

#include "benchmark/benchmark.h"

#include "Open3D/IO/ClassIO/PointCloudIO.h"
#include "Open3D/Utility/Console.h"

namespace open3d {
namespace benchmarks {

using open3d::io::ReadPointCloud;
using open3d::io::WritePointCloud;

namespace {

template <class T>
double AverageDistance(const std::vector<T> &a, const std::vector<T> &b) {
    if (a.size() != b.size()) {
        utility::LogError("vectors different size {} {}", a.size(), b.size());
    }
    double total = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        total += (a[i] - b[i]).norm();
    }
    return total / a.size();
}

enum class IsAscii : bool { BINARY = false, ASCII = true };
enum class Compressed : bool { UNCOMPRESSED = false, COMPRESSED = true };
enum class Compare : uint32_t {
    // Points are always compared
    NONE = 0,
    NORMALS = 1 << 0,
    COLORS = 1 << 1,
    NORMALS_AND_COLORS = NORMALS | COLORS
};
struct ReadWritePCArgs {
    std::string filename;
    IsAscii write_ascii;
    Compressed compressed;
    Compare compare;
};
std::vector<ReadWritePCArgs> g_pc_args({
        {"testbu.pcd", IsAscii::BINARY, Compressed::UNCOMPRESSED,
         Compare::NORMALS_AND_COLORS},  // 0
        {"testau.pcd", IsAscii::ASCII, Compressed::UNCOMPRESSED,
         Compare::NORMALS_AND_COLORS},  // 1
        {"testbc.pcd", IsAscii::BINARY, Compressed::COMPRESSED,
         Compare::NORMALS_AND_COLORS},  // 2
        {"testac.pcd", IsAscii::ASCII, Compressed::COMPRESSED,
         Compare::NORMALS_AND_COLORS},  // 3
        {"testb.ply", IsAscii::BINARY, Compressed::UNCOMPRESSED,
         Compare::NORMALS_AND_COLORS},  // 4
        {"testa.ply", IsAscii::ASCII, Compressed::UNCOMPRESSED,
         Compare::NORMALS_AND_COLORS},  // 5
        {"test.pts", IsAscii::BINARY, Compressed::UNCOMPRESSED,
         Compare::COLORS},  // 6
        {"test.xyz", IsAscii::BINARY, Compressed::UNCOMPRESSED,
         Compare::NONE},  // 7
        {"test.xyzn", IsAscii::BINARY, Compressed::UNCOMPRESSED,
         Compare::NORMALS},  // 8
        {"test.xyzrgb", IsAscii::BINARY, Compressed::UNCOMPRESSED,
         Compare::COLORS},  // 9
});

class TestPCGrid0 {
    constexpr static double step_ = .139;
    constexpr static double offset_ = -50.;
    geometry::PointCloud pc_;

    int size_ = 0;

public:
    void Setup(int size) {
        if (size_ == size) return;
        utility::LogInfo("setup PCGrid size={:d}", size);
        pc_.Clear();

        size_ = size;
        for (int i = 0; i < size; ++i) {
            double f = double(i) * step_ + offset_;
            pc_.points_.push_back({0., sin(f) * 1000., cos(f) * 1000.});
            pc_.normals_.push_back({sin(f), cos(f), 0.});
            double fc = fmod(double(i) * step_, 1.57);
            pc_.colors_.push_back({cos(fc), sin(fc), cos(fc)});
        }
    }

    void WriteRead(int pc_args_id) {
        const auto &args = g_pc_args[pc_args_id];
        const auto &pc = pc_;
        // we loose some precision when saving generated data
        if (!WritePointCloud(args.filename, pc, bool(args.write_ascii),
                             bool(args.compressed), false)) {
            utility::LogError("Failed to write to {}", args.filename);
        }
        geometry::PointCloud pc2;
        if (!ReadPointCloud(args.filename, pc2, "auto", false, false, false,
                            true)) {
            utility::LogError("Failed to read from {}", args.filename);
        }
        auto CheckLE = [](double a, double b) {
            if (a <= b) return;
            utility::LogError("Error too high: {} {}", a, b);
        };

        const double pointsMaxError =
                1e-3;  //.ply ascii has the highest error, others <1e-4
        CheckLE(AverageDistance(pc.points_, pc2.points_), pointsMaxError);
        if (int(args.compare) & int(Compare::NORMALS)) {
            const double normalsMaxError =
                    1e-6;  //.ply ascii has the highest error, others <1e-7
            CheckLE(AverageDistance(pc.normals_, pc2.normals_),
                    normalsMaxError);
        }
        if (int(args.compare) & int(Compare::COLORS)) {
            const double colorsMaxError =
                    1e-2;  // colors are saved as uint8_t[3] in a lot of formats
            CheckLE(AverageDistance(pc.colors_, pc2.colors_), colorsMaxError);
        }
    }
};
// reuse the same instance so we don't recreate the kdtree every time
TestPCGrid0 test_pc_grid0;

}  // namespace

static void BM_TestPCGrid0(::benchmark::State &state) {
    // state.range(n) are arguments that are passed to us
    int pc_args_id = state.range(0);
    int size = state.range(1);
    test_pc_grid0.Setup(size);
    for (auto _ : state) {
        test_pc_grid0.WriteRead(pc_args_id);
    }
}
static void BM_TestPCGrid0_Args(benchmark::internal::Benchmark *b) {
    for (int j = 4 * 1024; j <= 256 * 1024; j *= 8) {
        for (int i = 0; i < int(g_pc_args.size()); ++i) {
            b->Args({i, j});
        }
    }
}

BENCHMARK(BM_TestPCGrid0)->MinTime(0.1)->Apply(BM_TestPCGrid0_Args);

}  // namespace benchmarks
}  // namespace open3d
