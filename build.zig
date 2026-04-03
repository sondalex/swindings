const std = @import("std");
const zcc = @import("zig_compile_commands");

fn addSystemLibraryPaths(mod: *std.Build.Module) void {
    const system_lib_paths = [_][]const u8{
        "/usr/lib64",                // Fedora/RHEL
        "/usr/lib",                  // Arch/generic
        "/usr/lib/x86_64-linux-gnu", // Debian/Ubuntu
    };
    const system_include_paths = [_][]const u8{
        "/usr/include",
        "/usr/include/x86_64-linux-gnu", // Debian/Ubuntu
    };

    var threaded: std.Io.Threaded = .init_single_threaded;
    const io = threaded.io();

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
    const raylib_dep = b.dependency("raylib", .{
        .target = target,
        .optimize = optimize,
        .linux_display_backend = .Wayland

    });
    const raylib = raylib_dep.artifact("raylib");
    addSystemLibraryPaths(raylib.root_module); 

    const src_files = [_][]const u8{
        "src/main.c",
        "src/config.c",
        "src/keyicon.c",
        "src/display.c",
        "src/stringlist.c",
        "src/theme.c",
        "subprojects/tomlc17/src/tomlc17.c"
    };


    const exe = b.addExecutable(.{
        .name = "swindings",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    addSystemLibraryPaths(exe.root_module);
    exe.root_module.addCSourceFiles(.{
        .files=&src_files,
        .flags=&c_flags
    });
    

    exe.root_module.addIncludePath(b.path("src"));
    exe.root_module.addIncludePath(b.path("subprojects/raygui/src"));
    exe.root_module.addIncludePath(b.path("subprojects/asprintf"));
    exe.root_module.addIncludePath(b.path("subprojects/tomlc17/src"));

    exe.root_module.linkLibrary(raylib);

    exe.root_module.linkSystemLibrary("m", .{});
    b.installArtifact(exe);

    var targets = std.ArrayList(*std.Build.Step.Compile).empty;
    defer targets.deinit(b.allocator);

    try targets.append(b.allocator, exe);

    _ = zcc.createStep(b, "cdb", try targets.toOwnedSlice(b.allocator));

}
