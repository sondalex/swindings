import "./style.css";

const button: HTMLElement | null = document.getElementById("clipboard");

const first = <T>(values: ArrayLike<T>): T | null => {
    return values.length > 0 ? values[0] : null;
};

button?.addEventListener("click", async (_: Event) => {
    const code = first(document.getElementsByTagName("code"));
    if (!code || !button) return;

    try {
        await navigator.clipboard.writeText(code.textContent?.trim() ?? "");

        button.classList.add("clicked");

        setTimeout(() => {
            button.classList.remove("clicked");
        }, 200);
    } catch (err) {
        console.error("Failed to copy to clipboard:", err);
    }
});
