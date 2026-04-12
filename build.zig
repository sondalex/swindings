const std = @import("std");
const zcc = @import("zig_compile_commands");

const c_flags = [_][]const u8{
    "-std=c99",
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-D_POSIX_C_SOURCE=200809L",
};

const src_files = [_][]const u8{
    "src/main.c",
    "src/config.c",
    "src/keyicon.c",
    "src/display.c",
    "src/stringlist.c",
    "src/theme.c",
    "src/cli.c",
    "subprojects/tomlc17/src/tomlc17.c",
    "subprojects/cargs/src/cargs.c",
    "subprojects/fzy/src/match.c",
};

fn getGitVersion(b: *std.Build, io: std.Io) ![]const u8 {
    const tag_result = std.process.run(
        b.allocator,
        io,
        .{
            .argv = &.{ "git", "describe", "--tags", "--exact-match" },
        },
    ) catch |err| {
        if (err != error.ProcessTerminatedWithNonZeroExitCode) {
            std.debug.print("Warning: git describe --exact-match failed ({s})\n", .{@errorName(err)});
        }
        const hash_result = std.process.run(
            b.allocator,
            io,
            .{
                .argv = &.{ "git", "describe", "--tags", "--always" },
            },
        ) catch |hash_err| {
            std.debug.print("Warning: git describe --always failed ({s}), using fallback\n", .{@errorName(hash_err)});
            return error.GitFailed;
        };

        const trimmed_hash = std.mem.trim(u8, hash_result.stdout, " \n\r\t");
        return b.dupe(trimmed_hash);
    };

    const trimmed_tag = std.mem.trim(u8, tag_result.stdout, " \n\r\t");
    return b.dupe(trimmed_tag);
}

fn addSystemLibraryPaths(mod: *std.Build.Module, io: std.Io) void {
    const system_lib_paths = [_][]const u8{
        "/usr/lib64", // Fedora/RHEL
        "/usr/lib", // Arch/generic
        "/usr/lib/x86_64-linux-gnu", // Debian/Ubuntu
    };
    const system_include_paths = [_][]const u8{
        "/usr/include",
        "/usr/include/x86_64-linux-gnu", // Debian/Ubuntu
    };

    for (system_lib_paths) |path| {
        std.Io.Dir.cwd().access(io, path, .{}) catch continue;
        mod.addLibraryPath(.{ .cwd_relative = path });
    }
    for (system_include_paths) |path| {
        std.Io.Dir.cwd().access(io, path, .{}) catch continue;
        mod.addSystemIncludePath(.{ .cwd_relative = path });
    }
}

fn copyConfigHeader(b: *std.Build) !std.ArrayList(std.Build.LazyPath) {
    const wf = b.addWriteFiles();

    _ = wf.addCopyFile(
        b.path("subprojects/fzy/src/config.def.h"),
        "../config.h",
    );

    const config_dir: std.Build.LazyPath = wf.getDirectory();

    var include_paths: std.ArrayList(std.Build.LazyPath) = .empty;
    try include_paths.appendSlice(b.allocator, &.{
        b.path("subprojects/raygui/src"),
        b.path("subprojects/asprintf"),
        b.path("subprojects/tomlc17/src"),
        b.path("subprojects/cargs/include"),
        b.path("subprojects/fzy/src"),
    });

    try include_paths.append(b.allocator, config_dir);

    return include_paths;
}

fn addCIncludePaths(mod: *std.Build.Module, include_paths: std.ArrayList(std.Build.LazyPath)) void {
    for (include_paths.items) |path| {
        mod.addIncludePath(path);
    }
}

fn addCSourceFiles(mod: *std.Build.Module, src: []const []const u8, flags: []const []const u8) void {
    mod.addCSourceFiles(.{ .files = src, .flags = flags });
}

pub fn build(b: *std.Build) !void {
    var threaded: std.Io.Threaded = .init(b.allocator, .{});
    const io = threaded.io();

    // --- Step 1: Build Info ---
    const version = getGitVersion(b, io) catch "unknown-version";

    // --- Step 2: Target & Optimization ---
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // --- Step 3: Dependencies ---
    const raylib_dep = b.dependency("raylib", .{
        .target = target,
        .optimize = optimize,
        .linux_display_backend = .Wayland,
    });
    const raylib = raylib_dep.artifact("raylib");
    addSystemLibraryPaths(raylib.root_module, io);

    // --- Step 4: Executable ---
    const exe = b.addExecutable(.{
        .name = "swindings",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    // --- Step 5: Config & Include Paths ---
    const include_paths = try copyConfigHeader(b);
    exe.root_module.addCMacro("GIT_VERSION", b.fmt("\"{s}\"", .{version}));
    addSystemLibraryPaths(exe.root_module, io);
    addCIncludePaths(exe.root_module, include_paths);

    // --- Step 6: Source Files ---
    addCSourceFiles(exe.root_module, &src_files, &c_flags);

    // --- Step 7: Linking ---
    exe.root_module.linkLibrary(raylib);
    exe.root_module.linkSystemLibrary("m", .{});
    b.installArtifact(exe);

    // Unit tests

    const unit_test = b.addExecutable(.{
        .name = "unit_tests",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    var test_include_paths: std.ArrayList(std.Build.LazyPath) = .empty;
    try test_include_paths.appendSlice(b.allocator, &.{
        b.path("subprojects/unity/src/"),
        b.path("src"),
    });

    addCIncludePaths(unit_test.root_module, test_include_paths);

    addCSourceFiles(unit_test.root_module, &[_][]const u8{
        "tests/stringlist_test.c",
        "subprojects/unity/src/unity.c",
    }, &c_flags);
    b.installArtifact(unit_test);

    const run_unit_tests = b.addRunArtifact(unit_test);
    const test_step = b.step("test", "Run unit tests with Unity");
    test_step.dependOn(&run_unit_tests.step);

    // Generate compile commands
    var targets: std.ArrayList(*std.Build.Step.Compile) = .empty;

    defer targets.deinit(b.allocator);
    try targets.append(b.allocator, exe);
    try targets.append(b.allocator, unit_test);
    const cdb_step = zcc.createStep(b, "cdb", try targets.toOwnedSlice(b.allocator));
    cdb_step.dependOn(&exe.step);
}
