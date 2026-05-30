const std = @import("std");
const zcc = @import("zig_compile_commands");
const dz = @import("download_zip");

const c_flags = [_][]const u8{
    "-std=c99",
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-D_POSIX_C_SOURCE=200809L",
    "-Wshadow",
    // "-Wvla",
    "-Wno-vla",
    "-Wfloat-equal",
    "-Wdouble-promotion",
    "-Wformat=2",
    "-Wformat-truncation",
    "-Wundef",
    "-Wconversion",
    "-Wsign-conversion",
    "-Wnull-dereference",
    "-Wuninitialized",
    "-Winit-self",
    "-Wstrict-prototypes",
    "-Wold-style-definition",
};

const src_files = [_][]const u8{
    "src/utils.c",
    "src/main.c",
    "src/search.c",
    "src/config.c",
    "src/keyicon.c",
    "src/display.c",
    "src/structures.c",
    "src/theme.c",
    "src/cli.c",
};

const deps_files = [_][]const u8{
    "subprojects/tomlc17/src/tomlc17.c",
    "subprojects/cargs/src/cargs.c",
    "subprojects/fzy/src/match.c",
    "subprojects/asprintf/asprintf.c",
};

const debug_sanitizer_flags = [_][]const u8{
    "-fno-omit-frame-pointer",
    "-fsanitize=undefined",
    "-g3",
    // "-fsanitize=leak", # NOTE: valgrind is used instead
};

const raylib_disable_flags = [_][]const u8{
    "SUPPORT_TRACELOG", // NOTE: Remove this line for debugging
    "SUPPORT_MODULE_RMODELS",
    "SUPPORT_MODULE_RAUDIO",
    "SUPPORT_CAMERA_SYSTEM",
    "SUPPORT_GESTURES_SYSTEM",
    "SUPPORT_SCREEN_CAPTURE",
    "SUPPORT_IMAGE_EXPORT",
    "SUPPORT_IMAGE_GENERATION",
};

fn combineFlags(allocator: std.mem.Allocator, a: []const []const u8, b: []const []const u8) ![]const []const u8 {
    var list: std.ArrayList([]const u8) = .empty;
    try list.appendSlice(allocator, a);
    try list.appendSlice(allocator, b);
    return list.toOwnedSlice(allocator);
}

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

fn getIncludePaths(b: *std.Build) !std.ArrayList(std.Build.LazyPath) {
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
    const valgrind = b.option(bool, "valgrind", "Build with WITH_VALGRIND defined") orelse false;

    // --- Step 3: Dependencies ---
    const raylib_dep = b.dependency("raylib", .{
        .target = target,
        .optimize = optimize,
        .linux_display_backend = .Wayland,
    });
    const raylib = raylib_dep.artifact("raylib");
    for (raylib_disable_flags) |flag| {
        raylib.root_module.addCMacro(flag, "0");
    }

    addSystemLibraryPaths(raylib.root_module, io);

    // --- Step 4: Executable ---
    const exe = b.addExecutable(.{
        .name = "swindings",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .sanitize_c = if (optimize == .Debug) .full else .off,
        }),
    });

    const install_path = b.getInstallPath(.prefix, "share/swindings/fonts");

    _ = dz.addDownloadStep(
        b,
        "https://github.com/ryanoasis/nerd-fonts/releases/download/v3.4.0/JetBrainsMono.zip",
        install_path,
        "dz",
        "Download JetBrainsMono Font",
    );

    // --- Step 5: Config & Include Paths ---
    const include_paths = try getIncludePaths(b);
    exe.root_module.addCMacro("GIT_VERSION", b.fmt("\"{s}\"", .{version}));
    if (valgrind) exe.root_module.addCMacro("WITH_VALGRIND", "1");

    const prefix = b.install_prefix;
    const sysconfdir = if (std.mem.eql(u8, prefix, "/usr"))
        "/etc"
    else
        b.fmt("{s}/etc", .{prefix});

    exe.root_module.addCMacro("SYSCONFDIR", b.fmt("\"{s}\"", .{sysconfdir}));

    addSystemLibraryPaths(exe.root_module, io);
    addCIncludePaths(exe.root_module, include_paths);

    const exe_flags = try combineFlags(b.allocator, &c_flags, if (optimize == .Debug) &debug_sanitizer_flags else &.{});
    defer b.allocator.free(exe_flags);

    // --- Step 6: Source Files ---
    addCSourceFiles(exe.root_module, &src_files, exe_flags);
    addCSourceFiles(exe.root_module, &deps_files, &c_flags);

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
            .sanitize_c = if (optimize == .Debug) .full else .off,
        }),
    });

    var test_include_paths: std.ArrayList(std.Build.LazyPath) = .empty;
    try test_include_paths.appendSlice(b.allocator, &.{
        b.path("subprojects/unity/src"),
        b.path("src"),
        b.path("subprojects/tomlc17/src"),
        b.path("subprojects/asprintf"),
    });

    addCIncludePaths(unit_test.root_module, test_include_paths);

    const test_flags = try combineFlags(b.allocator, &c_flags, if (optimize == .Debug) &debug_sanitizer_flags else &.{});
    defer b.allocator.free(test_flags);

    addCSourceFiles(unit_test.root_module, &[_][]const u8{
        "tests/all_test.c",
        "tests/structures_test.c",
        "tests/theme_test.c",
        "tests/config_test.c",
        "tests/utils_test.c",
        "src/structures.c",
        "src/theme.c",
        "src/utils.c",
        "src/config.c",
    }, test_flags);
    addCSourceFiles(unit_test.root_module, &[_][]const u8{
        "subprojects/unity/src/unity.c",
        "subprojects/tomlc17/src/tomlc17.c",
        "subprojects/asprintf/asprintf.c",
    }, &c_flags);
    b.installArtifact(unit_test);
    if (valgrind) unit_test.root_module.addCMacro("WITH_VALGRIND", "1");

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
