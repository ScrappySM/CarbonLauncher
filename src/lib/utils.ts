import { clsx, type ClassValue } from "clsx"
import { twMerge } from "tailwind-merge"
import { invoke, convertFileSrc } from "@tauri-apps/api/core";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs))
}

// Interface for the image cache result from backend
interface ImageResult {
  cached: boolean;
  path: string;
}

// Image cache for storing already loaded image URLs in memory
const imageCache = new Map<string, string>();

/**
 * Gets a cached image or downloads and caches it
 * Uses a request queue to avoid too many requests at once
 */
export async function getCachedImage(imageUrl: string | undefined, modId: string): Promise<string> {
  // If URL is undefined/null/empty, return a placeholder
  if (!imageUrl) {
    return ""; // Return empty string to trigger fallback
  }
  
  // Check memory cache first
  if (imageCache.has(imageUrl)) {
    return imageCache.get(imageUrl)!;
  }
  
  try {
    // Validate URL format
    let validatedUrl = imageUrl;
    try {
      // Make sure URL is valid by constructing URL object
      new URL(imageUrl);
    } catch (e) {
      console.warn(`Invalid URL format for mod ${modId}:`, imageUrl);
      return imageUrl; // Return original if invalid
    }
    
    // Call the Rust backend to check file cache or download
    const result = await invoke<ImageResult>("get_or_cache_image", {
      imageUrl: validatedUrl,
      modId
    });
    
    // Convert the file path to a URL that the web view can access
    // Use convertFileSrc directly with the path from the result
    const fileUrl = convertFileSrc(result.path);
    
    // Store in memory cache
    imageCache.set(imageUrl, fileUrl);
    return fileUrl;
  } catch (error) {
    console.error("Failed to cache image:", error);
    // Return original URL as fallback
    return imageUrl;
  }
}

/**
 * Clears the image cache (both memory and disk)
 */
export async function clearImageCache(): Promise<void> {
  // Clear memory cache
  imageCache.clear();
  
  // Clear disk cache
  try {
    await invoke<string>("clear_image_cache");
  } catch (error) {
    console.error("Failed to clear image cache:", error);
  }
}
