import { createContext, useContext, useEffect, useState } from "react";

type Theme = "light" | "dark" | "system";
type AccentColor = "blue" | "green" | "purple" | "orange" | "red";

type ThemeProviderProps = {
  children: React.ReactNode;
  defaultTheme?: Theme;
  defaultAccentColor?: AccentColor;
  storageKey?: string;
  accentStorageKey?: string;
};

type ThemeProviderState = {
  theme: Theme;
  accentColor: AccentColor;
  setTheme: (theme: Theme) => void;
  setAccentColor: (color: AccentColor) => void;
  disableRounding: boolean;
  setDisableRounding: (disable: boolean) => void;
};

const ThemeProviderContext = createContext<ThemeProviderState | undefined>(
  undefined
);

export function ThemeProvider({
  children,
  defaultTheme = "system",
  defaultAccentColor = "blue",
  storageKey = "carbon-launcher-ui-theme",
  accentStorageKey = "carbon-launcher-ui-accent",
  ...props
}: ThemeProviderProps) {
  const [theme, setTheme] = useState<Theme>(() => {
    if (typeof window !== "undefined") {
      const storedTheme = localStorage.getItem(storageKey);
      return (storedTheme as Theme) || defaultTheme;
    }
    return defaultTheme;
  });

  const [accentColor, setAccentColorState] = useState<AccentColor>(() => {
    if (typeof window !== "undefined") {
      const storedAccent = localStorage.getItem(accentStorageKey);
      return (storedAccent as AccentColor) || defaultAccentColor;
    }
    return defaultAccentColor;
  });

  const setAccentColor = (color: AccentColor) => {
    setAccentColorState(color);
    if (typeof window !== "undefined") {
      localStorage.setItem(accentStorageKey, color);
    }
  };

  const [disableRounding, setDisableRoundingState] = useState<boolean>(() => {
    if (typeof window !== "undefined") {
      const stored = localStorage.getItem("carbon-launcher-disable-rounding");
      return stored === "true";
    }
    return false;
  });

  const setDisableRounding = (disable: boolean) => {
    setDisableRoundingState(disable);
    if (typeof window !== "undefined") {
      localStorage.setItem("carbon-launcher-disable-rounding", disable ? "true" : "false");
    }
  };

  useEffect(() => {
    const root = window.document.documentElement;
    root.classList.remove("light", "dark");

    // Apply theme
    if (theme === "system") {
      const systemTheme = window.matchMedia("(prefers-color-scheme: dark)")
        .matches
        ? "dark"
        : "light";

      root.classList.add(systemTheme);
      root.setAttribute("data-theme", systemTheme);
    } else {
      root.classList.add(theme);
      root.setAttribute("data-theme", theme);
    }
  }, [theme]);

  useEffect(() => {
    if (typeof window !== "undefined") {
      const root = window.document.documentElement;
      
      // Apply accent color
      root.style.setProperty('--primary', `var(--${accentColor})`);
      root.style.setProperty('--primary-foreground', `var(--${accentColor}-foreground)`);
      root.style.setProperty('--primary-rgb', `var(--${accentColor}-rgb)`);
      root.style.setProperty('--accent', `var(--${accentColor})`);
      root.style.setProperty('--accent-foreground', `var(--${accentColor}-foreground)`);
      root.style.setProperty('--ring', `var(--${accentColor})`);
      
      // Set data attribute for component styling
      root.setAttribute("data-accent", accentColor);
    }
  }, [accentColor]);

  useEffect(() => {
    if (typeof window !== "undefined") {
      localStorage.setItem(storageKey, theme);
    }
  }, [theme, storageKey]);

  useEffect(() => {
    if (typeof window !== "undefined") {
      const root = window.document.documentElement;
      if (disableRounding) {
        root.setAttribute("data-disable-rounding", "true");
        root.style.setProperty('--radius', '0px');
      } else {
        root.removeAttribute("data-disable-rounding");
        root.style.setProperty('--radius', '0.5rem'); // default value
      }
    }
  }, [disableRounding]);

  const value = {
    theme,
    accentColor,
    setTheme: (theme: Theme) => {
      setTheme(theme);
    },
    setAccentColor,
    disableRounding,
    setDisableRounding,
  };

  return (
    <ThemeProviderContext.Provider {...props} value={value}>
      {children}
    </ThemeProviderContext.Provider>
  );
}

function getEffectiveTheme(theme: "light" | "dark" | "system") {
  if (theme === "system") {
    if (typeof window !== "undefined" && window.matchMedia) {
      return window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
    }
    return "light";
  }
  return theme;
}

export const useTheme = (): ThemeProviderState & { effectiveTheme: "light" | "dark" } => {
  const context = useContext(ThemeProviderContext);
  if (context === undefined) {
    throw new Error("useTheme must be used within a ThemeProvider");
  }
  const effectiveTheme = getEffectiveTheme(context.theme);
  return { ...context, effectiveTheme };
};
