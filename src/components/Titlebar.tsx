import { useState, useEffect, useRef } from "react";
import { Minus, Maximize, X } from "lucide-react";
import { getCurrentWindow } from '@tauri-apps/api/window';

export interface TitlebarProps {
  title?: string;
}

export function Titlebar({ title = "Carbon Launcher" }: TitlebarProps) {
  const [isLoaded, setIsLoaded] = useState(false);
  const appWindow = useRef<any>(null);
  
  useEffect(() => {
    // Initialize the window reference
    const initWindow = async () => {
      try {
        appWindow.current = await getCurrentWindow();
      } catch (error) {
        console.error("Failed to get current window:", error);
      }
    };

    initWindow();
    setTimeout(() => setIsLoaded(true), 200);
  }, []);

  const handleMinimize = async () => {
    try {
      if (appWindow.current) {
        await appWindow.current.minimize();
      }
    } catch (error) {
      console.error("Failed to minimize window:", error);
    }
  };

  const handleMaximize = async () => {
    try {
      if (appWindow.current) {
        await appWindow.current.toggleMaximize();
      }
    } catch (error) {
      console.error("Failed to maximize/unmaximize window:", error);
    }
  };

  const handleClose = async () => {
    try {
      if (appWindow.current) {
        await appWindow.current.close();
      }
    } catch (error) {
      console.error("Failed to close window:", error);
    }
  };
  
  return (
    <div 
      data-tauri-drag-region 
      className={`h-9 bg-sidebar border-b border-sidebar-border flex items-center justify-between fixed top-0 left-0 right-0 z-[100] transition-opacity duration-500 ${isLoaded ? 'opacity-100' : 'opacity-0'}`}
    >
      <div className="flex items-center pl-4 h-full">
        <div className="text-foreground font-medium text-sm select-none">{title}</div>
      </div>
      
      <div className="flex items-center h-full">
        <button 
          id="titlebar-minimize"
          onClick={handleMinimize} 
          className="titlebar-button hover:bg-muted/50 flex items-center justify-center h-9 w-12 transition-colors"
        >
          <Minus className="h-4 w-4 text-foreground/70" />
        </button>
        <button 
          id="titlebar-maximize"
          onClick={handleMaximize} 
          className="titlebar-button hover:bg-muted/50 flex items-center justify-center h-9 w-12 transition-colors"
        >
          <Maximize className="h-4 w-4 text-foreground/70" />
        </button>
        <button 
          id="titlebar-close"
          onClick={handleClose} 
          className="titlebar-button hover:bg-destructive/90 flex items-center justify-center h-9 w-12 transition-colors"
        >
          <X className="h-4 w-4 text-foreground/70 hover:text-white" />
        </button>
      </div>
    </div>
  );
}