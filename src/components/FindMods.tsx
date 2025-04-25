import { useModsContext } from "../lib/mods";
import { Badge } from "./ui/badge";
import { Button } from "./ui/button";
import { Star, Download, RefreshCw, AlertOctagon, SearchIcon, Award, Sparkles, TrendingUp, ShieldCheck, ShieldAlert } from "lucide-react";
import { useState, useEffect } from "react";
import { CachedImage } from "./ui/cached-image";

export default function FindMods() {
    const [isLoaded, setIsLoaded] = useState(false);
    const [activeFilter, setActiveFilter] = useState("all");
    const [searchQuery, setSearchQuery] = useState("");
    const { getAvailableMods, isLoading, error, refreshModList, installMod, isModInstalling } = useModsContext();
    const availableMods = getAvailableMods().filter(mod => {
        if (searchQuery) {
            const searchLower = searchQuery.toLowerCase();
            return mod.name.toLowerCase().includes(searchLower) || 
                   mod.description.toLowerCase().includes(searchLower) ||
                   mod.author.toLowerCase().includes(searchLower);
        }
        return true; // Show all mods when no search query
    }).sort((a, b) => {
        if (activeFilter === "popular") {
            return (b.downloads || 0) - (a.downloads || 0);
        } else if (activeFilter === "rating") {
            return (b.stars || 0) - (a.stars || 0);
        }
        return 0;
    });

    useEffect(() => {
        const timer = setTimeout(() => setIsLoaded(true), 30);
        return () => clearTimeout(timer);
    }, []);

    // Map badge types to their visual representation and icon
    const getBadgeConfig = (badge: string) => {
        switch (badge?.toLowerCase()) {
            case 'popular':
                return { 
                    variant: 'popular' as const,
                    icon: <Download className="h-3 w-3" />,
                    label: 'Popular'
                };
            case 'trending':
                return {
                    variant: 'trending' as const,
                    icon: <TrendingUp className="h-3 w-3" />,
                    label: 'Trending'
                };
            case 'rating':
                return {
                    variant: 'rating' as const,
                    icon: <Star className="h-3 w-3" />,
                    label: 'Top Rated'
                };
            case 'new':
                return {
                    variant: 'new' as const,
                    icon: <Sparkles className="h-3 w-3" />,
                    label: 'New'
                };
            case 'featured':
                return {
                    variant: 'featured' as const,
                    icon: <Award className="h-3 w-3" />,
                    label: 'Featured'
                };
            default:
                return null; // Return null for undefined or unknown badge types
        }
    };

    if (isLoading && availableMods.length === 0) {
        return (
            <div className="h-full flex flex-col rounded-sm border border-border/60 bg-card text-card-foreground overflow-hidden">
                <div className="p-5 border-b border-border/40">
                    <h1 className="text-3xl font-bold mb-2 page-title">Find Mods</h1>
                    <p className="text-muted-foreground">Discover and install new mods</p>
                </div>
                <div className="flex-1 flex items-center justify-center">
                    <div className="flex flex-col items-center text-center p-8">
                        <RefreshCw className="h-10 w-10 animate-spin mb-4 text-primary" />
                        <h3 className="font-medium text-lg mb-2">Loading mods...</h3>
                        <p className="text-muted-foreground">Please wait while we fetch available mods</p>
                    </div>
                </div>
            </div>
        );
    }

    if (error) {
        return (
            <div className="h-full flex flex-col rounded-sm border border-border/60 bg-card text-card-foreground overflow-hidden">
                <div className="p-5 border-b border-border/40">
                    <h1 className="text-3xl font-bold mb-2 page-title">Find Mods</h1>
                    <p className="text-muted-foreground">Discover and install new mods</p>
                </div>
                <div className="flex-1 flex items-center justify-center">
                    <div className="flex flex-col items-center text-center p-8 max-w-md">
                        <AlertOctagon className="h-10 w-10 mb-4 text-red-500" />
                        <h3 className="font-medium text-lg mb-2">Error Loading Mods</h3>
                        <p className="text-muted-foreground mb-4">{error}</p>
                        <Button onClick={refreshModList}>Try Again</Button>
                    </div>
                </div>
            </div>
        );
    }

    return (
        <div className="h-full flex flex-col rounded-sm border border-border/60 bg-card text-card-foreground overflow-hidden">
            <div className="p-5 border-b border-border/40">
                <h1 className="text-3xl font-bold mb-2 page-title">Find Mods</h1>
                <p className="text-muted-foreground">Discover and install new mods</p>
            </div>
            <div className="px-5 py-4 border-b border-border/40">
                <div className="relative">
                    <input 
                        placeholder="Search for mods..." 
                        className="pl-10 pr-28 py-2 h-11 bg-muted/50 rounded-md border border-border w-full" 
                        value={searchQuery}
                        onChange={e => setSearchQuery(e.target.value)}
                    />
                    <SearchIcon className="absolute left-3 top-1/2 -translate-y-1/2 h-4 w-4 text-muted-foreground" />
                    <div className="absolute right-3 top-1/2 -translate-y-1/2 flex gap-2">
                        <Button size="sm" variant={activeFilter === "popular" ? "default" : "ghost"} className="h-7 px-2 text-xs" onClick={() => setActiveFilter("popular")}>
                            <Download className="h-3.5 w-3.5 mr-1" />
                            Popular
                        </Button>
                        <Button size="sm" variant={activeFilter === "rating" ? "default" : "ghost"} className="h-7 px-2 text-xs" onClick={() => setActiveFilter("rating")}>
                            <Star className="h-3.5 w-3.5 mr-1" />
                            Rating
                        </Button>
                        <Button size="sm" variant={activeFilter === "all" ? "default" : "ghost"} className="h-7 px-2 text-xs" onClick={() => setActiveFilter("all")}>All</Button>
                    </div>
                </div>
            </div>
            <div className="p-5 flex-1 overflow-auto">
                {availableMods.length === 0 ? (
                    <div className="flex-1 flex items-center justify-center h-full">
                        <div className="flex flex-col items-center text-center p-8 max-w-md">
                            <SearchIcon className="h-10 w-10 mb-4 text-muted-foreground" />
                            <h3 className="font-medium text-lg mb-2">No Mods Found</h3>
                            <p className="text-muted-foreground mb-4">
                                {searchQuery 
                                    ? `No mods match your search "${searchQuery}"`
                                    : `No mods found with filter "${activeFilter}"`}
                            </p>
                            <Button onClick={() => {
                                setSearchQuery("");
                                setActiveFilter("all");
                            }}>Clear Filters</Button>
                        </div>
                    </div>
                ) : (
                    <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 card-transition">
                        {availableMods.map((mod, i) => (
                            <div
                                key={mod.id}
                                className={`group rounded-sm flex flex-col bg-muted/30 border border-border/60 overflow-hidden hover:shadow-md transition-all ${isLoaded ? 'fade-in' : 'opacity-0'}`}
                                style={{ transitionDelay: `${i * 20}ms` }}
                            >
                                <div className="aspect-video bg-muted/50 relative overflow-hidden">
                                    <CachedImage
                                        src={mod.imageUrl}
                                        modId={mod.id}
                                        alt={mod.name}
                                        className="w-full h-full object-cover"
                                        fallback={
                                            <div className="absolute inset-0 flex items-center justify-center text-4xl font-bold opacity-20">
                                                {mod.name.charAt(0)}
                                            </div>
                                        }
                                    />
                                    {mod.badge && (() => {
                                        const badgeConfig = getBadgeConfig(mod.badge);
                                        if (!badgeConfig) return null; // Don't render if no badge config
                                        
                                        const { variant, icon, label } = badgeConfig;
                                        return (
                                            <div className="absolute top-2 right-2">
                                                <Badge variant={variant} className="shadow-sm">
                                                    {icon}
                                                    {label}
                                                </Badge>
                                            </div>
                                        );
                                    })()}
                                    {mod.isInstalled && mod.hashVerified === false && (
                                        <div className="absolute bottom-2 right-2">
                                            <Badge variant="unverified" className="shadow-sm">
                                                <ShieldAlert className="h-3 w-3" />
                                                Unverified
                                            </Badge>
                                        </div>
                                    )}
                                    {mod.isInstalled && mod.hashVerified === true && (
                                        <div className="absolute bottom-2 right-2">
                                            <Badge variant="verified" className="shadow-sm">
                                                <ShieldCheck className="h-3 w-3" />
                                                Verified
                                            </Badge>
                                        </div>
                                    )}
                                    <div className="absolute inset-0 bg-gradient-to-t from-black/50 to-transparent opacity-0 group-hover:opacity-100 transition-opacity"></div>
                                </div>
                                <div className="p-4 flex-1 flex flex-col">
                                    <h3 className="font-medium mb-1">{mod.name}</h3>
                                    <p className="text-sm text-muted-foreground mb-3 line-clamp-2">{mod.description}</p>
                                    <div className="flex items-center justify-between mt-auto">
                                        <div className="flex items-center text-sm">
                                            {mod.stars !== undefined && (
                                                <div className="flex items-center text-amber-400">
                                                    <Star className="h-3.5 w-3.5 fill-current" />
                                                    <span className="ml-1">{mod.stars}</span>
                                                </div>
                                            )}
                                            {mod.stars !== undefined && mod.downloads && <span className="mx-2 text-muted-foreground">•</span>}
                                            {mod.downloads && (
                                                <div className="flex items-center text-muted-foreground">
                                                    <Download className="h-3.5 w-3.5 mr-1" />
                                                    <span>{mod.downloads.toLocaleString()}</span>
                                                </div>
                                            )}
                                        </div>
                                        <Button 
                                            size="sm" 
                                            variant="default" 
                                            className="button-glow"
                                            onClick={() => installMod(mod.id)}
                                            disabled={isModInstalling(mod.id) || mod.isInstalled}
                                        >
                                            {isModInstalling(mod.id) ? "Installing..." : "Install"}
                                        </Button>
                                    </div>
                                </div>
                            </div>
                        ))}
                    </div>
                )}
            </div>
        </div>
    );
}
