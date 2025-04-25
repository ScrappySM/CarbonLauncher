import * as React from "react"
import { cva, type VariantProps } from "class-variance-authority"

import { cn } from "@/lib/utils"

const badgeVariants = cva(
  "inline-flex items-center rounded-md border px-2 py-0.5 text-xs font-medium transition-colors focus:outline-none focus:ring-2 focus:ring-ring focus:ring-offset-2 gap-1",
  {
    variants: {
      variant: {
        default:
          "border-transparent bg-primary text-primary-foreground shadow hover:bg-primary/80",
        secondary:
          "border-transparent bg-secondary text-secondary-foreground shadow hover:bg-secondary/80",
        destructive:
          "border-transparent bg-destructive text-destructive-foreground shadow hover:bg-destructive/80",
        outline: "text-foreground border-border",
        ghost: "border-transparent bg-primary/10 text-primary hover:bg-primary/20",
        popular: "border-transparent bg-blue-500/90 text-white shadow hover:bg-blue-500/80",
        trending: "border-transparent bg-orange-500/90 text-white shadow hover:bg-orange-500/80",
        new: "border-transparent bg-green-500/90 text-white shadow hover:bg-green-500/80",
        featured: "border-transparent bg-purple-500/90 text-white shadow hover:bg-purple-500/80",
        rating: "border-transparent bg-amber-500/90 text-white shadow hover:bg-amber-500/80",
        verified: "border-transparent bg-emerald-500/90 text-white shadow hover:bg-emerald-500/80",
        unverified: "border-transparent bg-red-500/90 text-white shadow hover:bg-red-500/80",
      },
      size: {
        default: "px-2 py-0.5 text-xs",
        sm: "px-1.5 py-0.25 text-[0.65rem]",
        lg: "px-2.5 py-0.75 text-sm",
      }
    },
    defaultVariants: {
      variant: "default",
      size: "default",
    },
  }
)

export interface BadgeProps
  extends React.HTMLAttributes<HTMLDivElement>,
    VariantProps<typeof badgeVariants> {}

function Badge({ className, variant, size, ...props }: BadgeProps) {
  return (
    <div className={cn(badgeVariants({ variant, size }), className)} {...props} />
  )
}

export { Badge, badgeVariants }