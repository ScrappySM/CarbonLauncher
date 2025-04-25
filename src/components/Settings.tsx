import { useTheme } from "../lib/theme";
import { Moon, Sun, Laptop, Square } from "lucide-react";
import { useState, useEffect } from "react";

const accentOptions = [
  { color: "blue", label: "Blue" },
  { color: "green", label: "Green" },
  { color: "purple", label: "Purple" },
  { color: "orange", label: "Orange" },
  { color: "red", label: "Red" },
];

const themeOptions = [
  { value: "system", label: "System", icon: <Laptop className="w-4 h-4" /> },
  { value: "light", label: "Light", icon: <Sun className="w-4 h-4" /> },
  { value: "dark", label: "Dark", icon: <Moon className="w-4 h-4" /> },
];

export default function Settings() {
  const { theme, setTheme, accentColor, setAccentColor, disableRounding, setDisableRounding } = useTheme();
  const [isLoaded, setIsLoaded] = useState(false);
  
  useEffect(() => {
    const timer = setTimeout(() => setIsLoaded(true), 30);
    return () => clearTimeout(timer);
  }, []);

  return (
    <div className="flex justify-center items-start pt-6 pb-12">
      <div className={`w-full max-w-2xl transition-all duration-500 ${isLoaded ? 'opacity-100 translate-y-0' : 'opacity-0 translate-y-4'}`}>
        <h1 className="text-4xl font-bold mb-6 page-title text-center">Settings</h1>
        
        <div className="bg-card border border-border/60 rounded-xl shadow-lg overflow-hidden mb-6">
          <div className="p-6 pb-4 border-b border-border/40">
            <h2 className="text-xl font-semibold flex items-center gap-2">
              <Sun className="w-5 h-5 text-primary" /> 
              <span>Appearance</span>
            </h2>
            <p className="text-muted-foreground text-sm mt-1">Customize how Carbon Launcher looks</p>
          </div>
          
          <div className="p-6">
            <div className="mb-8">
              <h3 className="text-base font-medium mb-3 text-foreground/90">Theme Mode</h3>
              <div className="flex gap-3">
                {themeOptions.map(opt => (
                  <button
                    key={opt.value}
                    className={`flex items-center gap-2 px-4 py-2.5 rounded-lg border transition-all duration-150 font-medium
                      ${theme === opt.value 
                        ? 'bg-primary text-primary-foreground border-primary shadow-sm' 
                        : 'bg-muted/50 text-foreground border-border hover:bg-accent/20'}`}
                    onClick={() => setTheme(opt.value as any)}
                    aria-pressed={theme === opt.value}
                  >
                    {opt.icon}
                    {opt.label}
                  </button>
                ))}
              </div>
            </div>
            
            <div className="mb-8">
              <h3 className="text-base font-medium mb-3 text-foreground/90">Corner Style</h3>
              <div className="flex gap-3">
                <button
                  className={`flex items-center gap-2 px-4 py-2.5 rounded-lg border transition-all duration-150 font-medium
                    ${!disableRounding 
                      ? 'bg-primary text-primary-foreground border-primary shadow-sm' 
                      : 'bg-muted/50 text-foreground border-border hover:bg-accent/20'}`}
                  onClick={() => setDisableRounding(false)}
                  aria-pressed={!disableRounding}
                >
                  <div className="w-4 h-4 rounded-md border-2 border-current flex-shrink-0"></div>
                  <span>Rounded</span>
                </button>
                <button
                  className={`flex items-center gap-2 px-4 py-2.5 rounded-lg border transition-all duration-150 font-medium
                    ${disableRounding 
                      ? 'bg-primary text-primary-foreground border-primary shadow-sm' 
                      : 'bg-muted/50 text-foreground border-border hover:bg-accent/20'}`}
                  onClick={() => setDisableRounding(true)}
                  aria-pressed={disableRounding}
                >
                  <Square className="w-4 h-4 flex-shrink-0" />
                  <span>Square</span>
                </button>
              </div>
            </div>
            
            <div>
              <h3 className="text-base font-medium mb-4 text-foreground/90">Accent Color</h3>
              <div className="grid grid-cols-5 gap-4">
                {accentOptions.map(opt => (
                  <button
                    key={opt.color}
                    className={`group flex flex-col items-center transition-all duration-150`}
                    onClick={() => setAccentColor(opt.color as any)}
                    aria-pressed={accentColor === opt.color}
                  >
                    <div className={`w-12 h-12 rounded-full shadow-sm mb-2 transition-transform group-hover:scale-105 ${accentColor === opt.color ? 'ring-4 ring-primary/30' : ''}`} 
                      style={{ background: `var(--${opt.color})` }}
                    />
                    <span className={`text-sm transition-colors ${accentColor === opt.color ? 'font-semibold text-foreground' : 'text-foreground/70 group-hover:text-foreground/90'}`}>
                      {opt.label}
                    </span>
                  </button>
                ))}
              </div>
            </div>
          </div>
        </div>
        
        <div className="bg-card border border-border/60 rounded-xl shadow-lg p-6">
          <div className="flex justify-between items-center">
            <div>
              <h2 className="text-xl font-semibold mb-1">Carbon Launcher</h2>
              <p className="text-sm text-muted-foreground">Version 0.1.0</p>
            </div>
            <button className="px-4 py-2 bg-primary/10 text-primary hover:bg-primary/20 rounded-lg transition-colors">
              Check for Updates
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
