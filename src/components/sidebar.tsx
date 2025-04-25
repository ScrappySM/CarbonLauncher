import { clsx } from "clsx";
import {
  Tooltip,
  TooltipTrigger,
  TooltipContent,
} from "@/components/ui/tooltip";
import { HomeIcon, SearchIcon, TerminalIcon, SettingsIcon } from "lucide-react";
import { useEffect, useState } from "react";

export interface SidebarProps {
  className?: string;
  onPageChange?: (page: string) => void;
  activePage?: string;
}

export function Sidebar({ className, onPageChange, activePage = "my-mods" }: SidebarProps) {
  const [isLoaded, setIsLoaded] = useState(false);
  
  useEffect(() => {
    setTimeout(() => setIsLoaded(true), 50);
  }, []);

  const navItems = [
    { id: "my-mods", label: "My Mods", icon: <HomeIcon className="h-5 w-5" /> },
    { id: "find-mods", label: "Find Mods", icon: <SearchIcon className="h-5 w-5" /> },
    { id: "games-console", label: "Games Console", icon: <TerminalIcon className="h-5 w-5" /> },
  ];

  return (
    <div
      className={clsx(
        "w-14 h-[calc(100vh-36px)] bg-sidebar border-r border-sidebar-border flex flex-col items-center py-6",
        "transition-all duration-200 ease-in-out fixed top-9 left-0 z-50",
        isLoaded ? "opacity-100" : "opacity-0 translate-x-[-10px]",
        className
      )}
    >
      <nav className="flex-1 flex flex-col items-center gap-3 w-full px-2">
        {navItems.map((item, index) => (
          <Tooltip key={item.id}>
            <TooltipTrigger asChild>
              <button
                onClick={() => onPageChange?.(item.id)}
                className={clsx(
                  "w-full h-10 rounded-md flex items-center justify-center relative overflow-hidden", 
                  "transition-all duration-150",
                  activePage === item.id 
                    ? "bg-primary text-white shadow-md" 
                    : "text-sidebar-foreground hover:bg-primary/10 hover:text-primary",
                  isLoaded ? "opacity-100" : "opacity-0",
                )}
                style={{ 
                  transitionDelay: `${(index + 1) * 30}ms`,
                  transform: isLoaded ? "translateY(0)" : "translateY(10px)"
                }}
              >
                {item.icon}
              </button>
            </TooltipTrigger>
            <TooltipContent 
              side="right" 
              sideOffset={6} 
              className="bg-popover text-popover-foreground border border-border shadow-md"
            >
              {item.label}
            </TooltipContent>
          </Tooltip>
        ))}
      </nav>
      
      <div className="mt-auto pt-4 w-full px-2">
        <Tooltip>
          <TooltipTrigger asChild>
            <button
              onClick={() => onPageChange?.("settings")}
              className={clsx(
                "w-full h-10 rounded-md flex items-center justify-center relative overflow-hidden", 
                "transition-all duration-150",
                activePage === "settings" 
                  ? "bg-primary text-white shadow-md" 
                  : "text-sidebar-foreground hover:bg-primary/10 hover:text-primary",
                isLoaded ? "opacity-100" : "opacity-0",
              )}
              style={{ 
                transitionDelay: "90ms",
                transform: isLoaded ? "translateY(0)" : "translateY(10px)"
              }}
            >
              <SettingsIcon className="h-5 w-5" />
            </button>
          </TooltipTrigger>
          <TooltipContent 
            side="right" 
            sideOffset={6} 
            className="bg-popover text-popover-foreground border border-border shadow-md"
          >
            Settings
          </TooltipContent>
        </Tooltip>
      </div>
    </div>
  );
}