import React from "react";
import ReactDOM from "react-dom/client";
import { ThemeProvider } from "./lib/theme";
import { ModsProvider } from "./lib/mods";
import App from "./App";

ReactDOM.createRoot(document.getElementById("root") as HTMLElement).render(
  <React.StrictMode>
    <ThemeProvider>
      <ModsProvider>
        <App />
      </ModsProvider>
    </ThemeProvider>
  </React.StrictMode>
);
