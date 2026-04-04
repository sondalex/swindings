const std = @import("std");
const zcc = @import("zig_compile_commands");

fn getGitVersion(b: *std.Build, io: std.Io) ![]const u8 { 
    const result = std.process.run(
        b.allocator,
        io,
        .{
        .argv = &.{ "git", "describe", "--tags",  "--always" },
    }) catch |err| {
        std.debug.print("Warning: git describe failed ({s}), using fallback\n", .{@errorName(err)});
        return error.GitFailed;
    };

    const trimmed = std.mem.trim(u8, result.stdout, " \n\r\t");
    return b.dupe(trimmed);
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

pub fn build(b: *std.Build) !void {
    var threaded: std.Io.Threaded = .init(b.allocator, .{});
    const io = threaded.io();

    const version = getGitVersion(b, io) catch "unknown-version";

    const c_flags = [_][]const u8{
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-pedantic",
        "-D_POSIX_C_SOURCE=200809L",
    };
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // External build: raylib submodule (using raylib's own build.zig)
    const raylib_dep = b.dependency("raylib", .{ .target = target, .optimize = optimize, .linux_display_backend = .Wayland });
    const raylib = raylib_dep.artifact("raylib");
    addSystemLibraryPaths(raylib.root_module, io);

    const src_files = [_][]const u8{ "src/main.c", "src/config.c", "src/keyicon.c", "src/display.c", "src/stringlist.c", "src/theme.c", "src/cli.c", "subprojects/tomlc17/src/tomlc17.c", "subprojects/cargs/src/cargs.c" };

    const exe = b.addExecutable(.{
        .name = "swindings",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    exe.root_module.addCMacro("GIT_VERSION", b.fmt("\"{s}\"", .{version}));
    addSystemLibraryPaths(exe.root_module, io);
    exe.root_module.addCSourceFiles(.{ .files = &src_files, .flags = &c_flags });

    exe.root_module.addIncludePath(b.path("src"));
    exe.root_module.addIncludePath(b.path("subprojects/raygui/src"));
    exe.root_module.addIncludePath(b.path("subprojects/asprintf"));
    exe.root_module.addIncludePath(b.path("subprojects/tomlc17/src"));
    exe.root_module.addIncludePath(b.path("subprojects/cargs/include"));

    exe.root_module.linkLibrary(raylib);

    exe.root_module.linkSystemLibrary("m", .{});
    b.installArtifact(exe);

    var targets = std.ArrayList(*std.Build.Step.Compile).empty;
    defer targets.deinit(b.allocator);

    try targets.append(b.allocator, exe);

    _ = zcc.createStep(b, "cdb", try targets.toOwnedSlice(b.allocator));
}
