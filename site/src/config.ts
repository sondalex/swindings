export const siteConfig = {
    title: "Swindings",
    description: "View sway keybindings",
    githubUrl: "https://github.com/sondalex/swindings",
    installCommand:
        "curl -sSfL https://sondalex.github.io/swindings/install.sh | sh",
    author: "sondalex",
    authorProfileURL: "https://github.com/sondalex",
    basePath: process.env.NODE_ENV === "production" ? "/swindings/" : "/",

    license: "MIT",
} as const;

export type SiteConfig = typeof siteConfig;
