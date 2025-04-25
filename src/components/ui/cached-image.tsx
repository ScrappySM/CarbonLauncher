import React, { useState, useEffect, useRef } from 'react';
import { getCachedImage } from '@/lib/utils';

interface CachedImageProps {
  src: string | undefined;
  modId: string;
  alt: string;
  className?: string;
  fallback?: React.ReactNode;
}

/**
 * CachedImage component that loads images from local cache or downloads and caches them
 * Uses intersection observer for lazy loading to prevent too many requests at once
 */
export function CachedImage({ 
  src, 
  modId, 
  alt, 
  className = '', 
  fallback
}: CachedImageProps) {
  const [imageSrc, setImageSrc] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [isError, setIsError] = useState(false);
  const [isIntersecting, setIsIntersecting] = useState(false);
  const imgRef = useRef<HTMLImageElement>(null);
  
  // Set up intersection observer to detect when the image comes into view
  useEffect(() => {
    const observer = new IntersectionObserver(
      (entries) => {
        // When the image becomes visible, set isIntersecting to true
        const entry = entries[0];
        setIsIntersecting(entry.isIntersecting);
      },
      {
        rootMargin: '200px', // Load images a bit before they come into view
        threshold: 0.01,     // Trigger when at least 1% of the element is visible
      }
    );
    
    if (imgRef.current) {
      observer.observe(imgRef.current);
    }
    
    return () => {
      observer.disconnect();
    };
  }, []);
  
  // Load the image when it comes into view
  useEffect(() => {
    if (!src || !isIntersecting) return;
    
    const loadImage = async () => {
      try {
        setIsLoading(true);
        const cachedSrc = await getCachedImage(src, modId);
        setImageSrc(cachedSrc);
        setIsError(false);
      } catch (error) {
        console.error('Failed to load image:', error);
        setIsError(true);
      } finally {
        setIsLoading(false);
      }
    };
    
    loadImage();
  }, [src, modId, isIntersecting]);
  
  // Show a placeholder or fallback during loading
  if (isLoading || !imageSrc) {
    return (
      <div 
        ref={imgRef}
        className={`${className} flex items-center justify-center bg-muted/50`}
        aria-label={isLoading ? "Loading image..." : alt}
      >
        {fallback || (
          <div className="absolute inset-0 flex items-center justify-center text-4xl font-bold opacity-20">
            {alt.charAt(0).toUpperCase()}
          </div>
        )}
      </div>
    );
  }
  
  // Show fallback if there was an error loading the image
  if (isError) {
    return (
      <div 
        className={`${className} flex items-center justify-center bg-muted/50`}
      >
        {fallback || (
          <div className="absolute inset-0 flex items-center justify-center text-4xl font-bold opacity-20">
            {alt.charAt(0).toUpperCase()}
          </div>
        )}
      </div>
    );
  }
  
  // Show the actual image
  return (
    <img
      ref={imgRef}
      src={imageSrc}
      alt={alt}
      className={className}
      onError={() => setIsError(true)}
    />
  );
}