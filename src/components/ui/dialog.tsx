import * as React from "react";
import { cn } from "@/lib/utils";
import { useEffect, useRef } from "react";
import { AlertTriangle, X } from "lucide-react";

export interface DialogProps {
  open: boolean;
  onClose: () => void;
  title?: string;
  children: React.ReactNode;
  variant?: "default" | "warning" | "error";
  maxWidth?: string;
  showCloseButton?: boolean;
}

export function Dialog({
  open,
  onClose,
  title,
  children,
  variant = "default",
  maxWidth = "md:max-w-lg",
  showCloseButton = true,
}: DialogProps) {
  const backdropRef = useRef<HTMLDivElement>(null);
  const dialogRef = useRef<HTMLDivElement>(null);

  // Handle escape key to close dialog
  useEffect(() => {
    const handleEscapeKey = (e: KeyboardEvent) => {
      if (open && e.key === "Escape") {
        onClose();
      }
    };

    document.addEventListener("keydown", handleEscapeKey);
    return () => document.removeEventListener("keydown", handleEscapeKey);
  }, [open, onClose]);

  // Handle click outside to close
  useEffect(() => {
    const handleBackdropClick = (e: MouseEvent) => {
      if (backdropRef.current && e.target === backdropRef.current) {
        onClose();
      }
    };

    if (open) {
      document.addEventListener("mousedown", handleBackdropClick);
      document.body.style.overflow = "hidden"; // Prevent scrolling when dialog is open
    }

    return () => {
      document.removeEventListener("mousedown", handleBackdropClick);
      document.body.style.overflow = ""; // Restore scrolling when dialog closes
    };
  }, [open, onClose]);

  // Animation on open
  useEffect(() => {
    if (open && dialogRef.current) {
      dialogRef.current.classList.add("dialog-enter");
      setTimeout(() => {
        if (dialogRef.current) {
          dialogRef.current.classList.remove("dialog-enter");
        }
      }, 300);
    }
  }, [open]);

  if (!open) return null;

  const variantStyles = {
    default: {
      header: "border-b border-border/40 bg-muted/20",
      body: "",
      icon: null,
    },
    warning: {
      header: "border-b border-orange-500/30 bg-orange-500/10",
      body: "",
      icon: <AlertTriangle className="h-5 w-5 text-orange-500 mr-2" />,
    },
    error: {
      header: "border-b border-destructive/30 bg-destructive/10",
      body: "",
      icon: <AlertTriangle className="h-5 w-5 text-destructive mr-2" />,
    },
  };

  const { header, body, icon } = variantStyles[variant];

  return (
    <div
      ref={backdropRef}
      className="fixed inset-0 z-50 bg-black/50 backdrop-blur-sm flex items-center justify-center p-4"
      aria-hidden="true"
    >
      <div
        ref={dialogRef}
        className={cn(
          "bg-card text-card-foreground rounded-lg shadow-lg w-full",
          maxWidth,
          "border border-border/60 overflow-hidden"
        )}
        role="dialog"
        aria-modal="true"
      >
        {title && (
          <div className={cn("flex items-center justify-between px-6 py-4", header)}>
            <div className="flex items-center">
              {icon}
              <h2 className="text-lg font-semibold">{title}</h2>
            </div>
            {showCloseButton && (
              <button
                onClick={onClose}
                className="rounded-sm opacity-70 hover:opacity-100 focus:outline-none focus:ring-1 focus:ring-primary"
              >
                <X className="h-4 w-4" />
                <span className="sr-only">Close</span>
              </button>
            )}
          </div>
        )}
        <div className={cn("p-6", body)}>{children}</div>
      </div>
    </div>
  );
}

export interface DialogFooterProps {
  children: React.ReactNode;
  className?: string;
}

export function DialogFooter({ children, className }: DialogFooterProps) {
  return (
    <div
      className={cn(
        "flex flex-col-reverse sm:flex-row sm:justify-end sm:space-x-2 mt-4 pt-4 border-t border-border/40",
        className
      )}
    >
      {children}
    </div>
  );
}

// Add some styling to App.css for animations
const styleElement = document.createElement("style");
styleElement.textContent = `
  .dialog-enter {
    animation: dialogEnter 0.3s cubic-bezier(0.16, 1, 0.3, 1);
  }
  
  @keyframes dialogEnter {
    from {
      opacity: 0;
      transform: scale(0.95);
    }
    to {
      opacity: 1;
      transform: scale(1);
    }
  }
`;
document.head.appendChild(styleElement);