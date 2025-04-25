import { useEffect, useState } from "react";

export default function GamesConsole() {
  const [isLoaded, setIsLoaded] = useState(false);
  useEffect(() => {
    const timer = setTimeout(() => setIsLoaded(true), 30);
    return () => clearTimeout(timer);
  }, []);
  return (
    <div className={`h-full flex flex-col items-center justify-center ${isLoaded ? 'fade-in' : 'opacity-0'}`}>
      <h1 className="text-3xl font-bold mb-6 page-title">Games Console</h1>
      <p className="text-muted-foreground text-lg">This feature is coming soon!</p>
    </div>
  );
}
