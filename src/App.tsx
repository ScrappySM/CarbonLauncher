import { useState } from "react";
import { useTheme } from "./lib/theme";
import { Toaster } from 'sonner';
import "./App.css";

import MyMods from "./components/MyMods";
import FindMods from "./components/FindMods";
import GamesConsole from "./components/GamesConsole";
import Settings from "./components/Settings";
import { Sidebar } from "./components/sidebar";
import { Titlebar } from "./components/Titlebar";

function getEffectiveTheme(theme: "light" | "dark" | "system") {
  if (theme === "system") {
    if (typeof window !== "undefined" && window.matchMedia) {
      return window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
    }
    return "light";
  }
  return theme;
}

function App() {
    const [activePage, setActivePage] = useState("my-mods");
    const { theme } = useTheme();
    const effectiveTheme = getEffectiveTheme(theme);

    // Render the active page content based on state
    const renderActivePageContent = () => {
        switch (activePage) {
            case "my-mods":
                return <MyMods onFindMods={() => setActivePage("find-mods")} />;
            case "find-mods":
                return <FindMods />;
            case "games-console":
                return <GamesConsole />;
            case "settings":
                return <Settings />;
            default:
                return <MyMods />;
        }
    };

    return (
        <>
            <Toaster position="bottom-right" theme={effectiveTheme} richColors closeButton visibleToasts={6} />
            <div className="flex min-h-screen w-full bg-background">
                <Titlebar title="Carbon Launcher" />
                <Sidebar
                    onPageChange={setActivePage}
                    activePage={activePage}
                />
                <div className="flex-1 flex ml-14 pt-12 bg-muted/10 p-4">
                    <div className="w-full h-[calc(100vh-48px-1rem)] max-w-[2200px] mx-auto">
                        {renderActivePageContent()}
                    </div>
                </div>
            </div>
        </>
    );
}

export default App;
