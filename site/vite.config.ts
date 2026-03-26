import { defineConfig } from "vite";
import { siteConfig } from "./src/config";
import { execSync } from "node:child_process";

import tailwindcss from "@tailwindcss/vite";
function getGitVersion(): string {
    try {
        return execSync("git describe --tags --abbrev=0", {
            encoding: "utf-8",
        }).trim();
    } catch {
        return "dev";
    }
}

export default defineConfig({
    base: siteConfig.basePath,
    plugins: [
        tailwindcss(),
        {
            name: "html-template",
            transformIndexHtml(html) {
                const config = { ...siteConfig, version: getGitVersion() };
                return html.replace(
                    /\{\{\s*(\w+)\s*\}\}/g,
                    (match, key) => config[key as keyof typeof config] ?? match,
                );
            },
        },
    ],
});
