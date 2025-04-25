import { createContext, useContext, useState, useEffect, ReactNode, useRef } from "react";
import { join } from '@tauri-apps/api/path';
import { invoke } from '@tauri-apps/api/core';
import { fetch } from '@tauri-apps/plugin-http';
import { mkdir, exists, readTextFile, writeTextFile } from '@tauri-apps/plugin-fs';
import { appDataDir } from '@tauri-apps/api/path';
import { save } from '@tauri-apps/plugin-dialog';
import { toast } from 'sonner';
import { HashVerificationWarning } from '@/components/HashVerificationWarning';

interface RepoMod {
    name: string;
    full_name: string;
    url: string;
    description: string;
    downloads: {
        name: string;
        url: string;
        currentHash: string;
        validHash: string;
        hashMatch: boolean;
    }[];
    total_downloads: number;
    stars: number;
    contributors: string[];
    icon: string;
    mismatched_hashes: any[];
}

export interface Mod {
    id: string;
    name: string;
    description: string;
    version: string;
    author: string;
    downloads: number;
    stars?: number;
    rating?: string;
    badge?: string;
    isInstalled: boolean;
    category?: string[];
    releaseDate?: string;
    lastUpdated?: string;
    size?: string;
    dependencies?: string[];
    imageUrl?: string;
    downloadFiles: { name: string; url: string; validHash?: string }[];
    repoUrl?: string;
    hashVerified?: boolean;
}

interface ModUpdateInfo {
    modId: string;
    currentVersion: string;
    latestVersion: string;
    downloadUrl: string;
    changelogUrl?: string;
}

interface ModsContextType {
    mods: Mod[];
    isLoading: boolean;
    error: string | null;
    refreshModList: () => Promise<void>;
    installMod: (modId: string) => Promise<void>;
    uninstallMod: (modId: string) => Promise<void>;
    getInstalledMods: () => Mod[];
    getAvailableMods: () => Mod[];
    downloadModList: () => Promise<void>;
    checkForModUpdates: () => Promise<ModUpdateInfo[]>;
    updateMod: (modId: string) => Promise<boolean>;
    lastUpdated: Date | null;
    installingModIds: Set<string>;
    isModInstalling: (modId: string) => boolean;
}

const ModsContext = createContext<ModsContextType | undefined>(undefined);
const REPO_URL = "https://raw.githubusercontent.com/ScrappySM/CarbonRepo/refs/heads/main/repos-gen2.json";

const CACHE_DIR = 'cache';
const MODS_CACHE_FILE = `${CACHE_DIR}/mods-cache.json`;

interface CacheData {
    timestamp: string;
    data: Mod[];
}

async function getCacheDir() {
    const appDataDirPath = await appDataDir();
    return await join(appDataDirPath, CACHE_DIR);
}
async function getModsCacheFile() {
    const appDataDirPath = await appDataDir();
    return await join(appDataDirPath, MODS_CACHE_FILE);
}

export function ModsProvider({ children }: { readonly children: ReactNode }) {
    const [mods, setMods] = useState<Mod[]>([]);
    const [isLoading, setIsLoading] = useState(true);
    const [error, setError] = useState<string | null>(null);
    const [lastUpdated, setLastUpdated] = useState<Date | null>(null);
    const [installingModIds, setInstallingModIds] = useState<Set<string>>(new Set());
    const isRefreshing = useRef(false);
    
    // Track the most up-to-date state of installations to prevent race conditions
    const installedModsRef = useRef<Set<string>>(new Set());
    const installationQueue = useRef<Promise<void>>(Promise.resolve());
    
    // State for hash verification warning modal
    const [hashWarningVisible, setHashWarningVisible] = useState(false);
    const [, setHashWarningModId] = useState<string>("");
    const [hashWarningModName, setHashWarningModName] = useState<string>("");
    const [pendingInstallAction, setPendingInstallAction] = useState<{
        modId: string;
        continueInstall: boolean;
        hashVerificationFailed: boolean;
        loadingToastId?: string | number;
    } | null>(null);

    // Initialize cache directory and load cached data on component mount
    useEffect(() => {
        const initializeCache = async () => {
            try {
                const cacheDirPath = await getCacheDir();
                const cacheDirExists = await exists(cacheDirPath);
                if (!cacheDirExists) {
                    await mkdir(cacheDirPath, { recursive: true });
                    console.log("Created cache directory");
                }
            } catch (err) {
                console.error("Failed to initialize cache directory:", err);
            }

            await loadCachedModsAndRefresh();
        };

        initializeCache();
    }, []);

    // When mods change, update our reference of installed mods
    useEffect(() => {
        const installedIds = new Set(mods.filter(mod => mod.isInstalled).map(mod => mod.id));
        installedModsRef.current = installedIds;
    }, [mods]);

    // Convert repository data to our mod format
    const convertRepoDataToMods = (repoMods: RepoMod[]): Mod[] => {
        const averageStars = (repoMods.reduce((acc, mod) => acc + mod.stars, 0) / repoMods.length);
        const averageDownloads = (repoMods.reduce((acc, mod) => acc + mod.total_downloads, 0) / repoMods.length);

        return repoMods.map((repoMod) => {
            const id = `${repoMod.full_name.toLowerCase().replace(/[^a-z0-9]/g, '-')}`;
            
            let badge: string | undefined;
            if (repoMod.stars >= averageStars * 1.25 || repoMod.total_downloads >= averageDownloads * 1.25) {
                badge = 'popular';
            } else {
                badge = undefined;
            }

            // Create download files array from the downloads property with hash information
            const downloadFiles = repoMod.downloads.map(download => ({ 
                name: download.name, 
                url: download.url,
                validHash: download.validHash
            }));
            
            // Try to extract version from download URL or use a default
            const version = downloadFiles[0]?.url.split('/').pop()?.split(/[-_v]/).filter(Boolean).pop()?.split('.')[0] ?? '1.0.0';

            return {
                id,
                name: repoMod.name,
                description: repoMod.description,
                version,
                author: repoMod.contributors.join(', '),
                downloads: repoMod.total_downloads,
                stars: repoMod.stars,
                isInstalled: false,
                repoUrl: repoMod.url,
                downloadFiles,
                badge,
                imageUrl: repoMod.icon
            };
        });
    };

    // Read mods data from cache file
    const loadCachedMods = async (): Promise<Mod[] | null> => {
        try {
            const cacheFilePath = await getModsCacheFile();
            const cacheFileExists = await exists(cacheFilePath);
            if (!cacheFileExists) {
                console.log("No cache file found");
                return null;
            }

            const cacheContent = await readTextFile(cacheFilePath);
            const cacheData = JSON.parse(cacheContent) as CacheData;

            console.log(`Loaded cached mods data from ${cacheData.timestamp}`);
            setLastUpdated(new Date(cacheData.timestamp));

            return cacheData.data;
        } catch (err) {
            console.error("Failed to load cached mods:", err);
            return null;
        }
    };

    // Save mods data to cache file
    const saveCachedMods = async (modsToCache: Mod[] | ((prevMods: Mod[]) => Mod[])) => {
        try {
            // Read the most current cached data first
            const currentCachedMods = await loadCachedMods() || [];
            
            // Apply the changes, either using the provided array directly or the update function
            const finalMods = typeof modsToCache === 'function' 
                ? modsToCache(currentCachedMods)
                : modsToCache;
            
            const timestamp = new Date().toISOString();
            const cacheData: CacheData = {
                timestamp,
                data: finalMods
            };
            
            const cacheFilePath = await getModsCacheFile();
            await writeTextFile(cacheFilePath, JSON.stringify(cacheData, null, 2));
            setLastUpdated(new Date(timestamp));
            console.log("Saved mods data to cache with timestamp:", timestamp);
        } catch (err) {
            console.error("Failed to save mods cache:", err);
        }
    };

    // Load cached mods and refresh from repository
    const loadCachedModsAndRefresh = async () => {
        if (isRefreshing.current) return;
        isRefreshing.current = true;
        setIsLoading(true);
        setError(null);

        const cachedMods = await loadCachedMods();
        if (cachedMods && cachedMods.length > 0) {
            setMods(cachedMods);
            toast.info("Loaded mods from cache. Displaying cached data while refreshing...");
        }

        // Use toast.promise for the fetch
        await toast.promise(
            (async () => {
                const response = await fetch(REPO_URL, { method: 'GET' });
                if (!response.ok) {
                    throw new Error(`Failed to fetch mods: ${response.status} ${response.statusText}`);
                }
                const repoData = await response.json() as RepoMod[];
                const fetchedMods = convertRepoDataToMods(repoData);

                const currentInstalledMap = new Map(
                    (cachedMods ?? []).filter(m => m.isInstalled).map(m => [m.id, m.version])
                );

                // Create merged mods that preserves installation status
                const mergedMods = fetchedMods.map(mod => ({
                    ...mod,
                    isInstalled: currentInstalledMap.has(mod.id) || installedModsRef.current.has(mod.id)
                }));

                setMods(mergedMods);
                // Use a direct array here since we're replacing the entire cache
                await saveCachedMods(mergedMods);
                return { count: mergedMods.length };
            })(),
            {
                loading: 'Fetching repository...',
                success: (data) => `Fetched ${data.count} mods from repository!`,
                error: 'Failed to fetch repository.'
            }
        );

        isRefreshing.current = false;
        setIsLoading(false);
    };

    const refreshModList = async () => {
        await loadCachedModsAndRefresh();
    };

    const isModInstalling = (modId: string) => installingModIds.has(modId);

    const installMod = async (modId: string) => {
        // First, check if the mod is already being installed
        if (installingModIds.has(modId)) {
            return; // Already installing this mod
        }
        
        // Add mod to installing set immediately for UI feedback
        setInstallingModIds(prev => new Set(prev).add(modId));
        
        // Queue this installation to prevent race conditions
        installationQueue.current = installationQueue.current.then(async () => {
            setError(null);
            let modToInstall: Mod | undefined;
            let hashVerificationFailed = false;
            let loadingToastId: string | number | undefined;
            
            try {
                modToInstall = mods.find(m => m.id === modId);
                if (!modToInstall || !modToInstall.downloadFiles.length) {
                    throw new Error(`Mod '${modId}' not found or has no download files.`);
                }
                
                // Create a toast with progress tracking
                loadingToastId = toast.loading(
                    <div className="flex flex-col">
                        <span>Installing {modToInstall.name}...</span>
                        <span className="text-xs text-muted-foreground">Preparing download...</span>
                    </div>
                );
                
                const results = [];
                // Process each download file sequentially
                for (let i = 0; i < modToInstall.downloadFiles.length; i++) {
                    const file = modToInstall.downloadFiles[i];
                    const fileProgress = i + 1;
                    const totalFiles = modToInstall.downloadFiles.length;
                    
                    // Update the loading toast with the current file being processed
                    if (loadingToastId) {
                        toast.loading(
                            <div className="flex flex-col">
                                <span>Installing {modToInstall.name}...</span>
                                <span className="text-xs text-muted-foreground">
                                    Downloading file {fileProgress} of {totalFiles}: {file.name}
                                </span>
                            </div>,
                            { id: loadingToastId }
                        );
                    }
                    
                    // Invoke the installation process
                    const result = await invoke("install_mod", {
                        modId: modToInstall.id,
                        name: modToInstall.name,
                        downloadUrl: file.url,
                        downloadName: file.name,
                        expectedHash: file.validHash || ""
                    });
                    
                    // Cast the result to include the properties we expect
                    const installResult = result as {
                        success: boolean;
                        hash_verified: boolean;
                        expected_hash: string;
                        actual_hash: string;
                        message: string;
                    };
                    
                    results.push(installResult);
                    
                    // Check hash verification
                    if (installResult.expected_hash && !installResult.hash_verified) {
                        hashVerificationFailed = true;
                    }
                    
                    // Update the toast with verification status for each file
                    if (loadingToastId && !hashVerificationFailed) {
                        toast.loading(
                            <div className="flex flex-col">
                                <span>Installing {modToInstall.name}...</span>
                                <span className="text-xs text-muted-foreground">
                                    {i === modToInstall.downloadFiles.length - 1 
                                        ? 'Finalizing installation...'
                                        : `File ${fileProgress} processed. Continuing...`}
                                </span>
                            </div>,
                            { id: loadingToastId }
                        );
                    }
                }
                
                // If hash verification failed, show the warning modal
                if (hashVerificationFailed) {
                    if (loadingToastId) toast.dismiss(loadingToastId);
                    
                    // Show the hash verification warning modal
                    setHashWarningModId(modId);
                    setHashWarningModName(modToInstall.name);
                    setHashWarningVisible(true);
                    
                    // Set pending install action
                    setPendingInstallAction({
                        modId,
                        continueInstall: false,
                        hashVerificationFailed,
                        loadingToastId
                    });
                    
                    // We'll update the mod status when the user makes a decision
                    // But we still need to clean up the installing state
                    setInstallingModIds(prev => {
                        const next = new Set(prev);
                        next.delete(modId);
                        return next;
                    });
                    return;
                }
                
                // If we get here, hash verification passed or wasn't required
                // Update mod status while preserving other installed mods
                setMods(prevMods => {
                    return prevMods.map(prevMod => {
                        if (prevMod.id === modId) {
                            return { 
                                ...prevMod, 
                                isInstalled: true,
                                hashVerified: !hashVerificationFailed
                            };
                        }
                        return prevMod;
                    });
                });
                
                // Add to installed mods reference
                installedModsRef.current.add(modId);
                
                // Update the cache with the latest state
                await saveCachedMods(prevMods => prevMods.map(prevMod =>
                    prevMod.id === modId ? { 
                        ...prevMod, 
                        isInstalled: true,
                        hashVerified: !hashVerificationFailed
                    } : prevMod
                ));
                
                // Show success toast
                if (loadingToastId) toast.dismiss(loadingToastId);
                toast.success(`${modToInstall.name} installed successfully`);
            } catch (err: any) {
                console.error(`Failed to install mod ${modId}:`, err);
                
                // Show error toast
                if (loadingToastId) toast.dismiss(loadingToastId);
                toast.error(`Failed to install ${modToInstall?.name ?? modId}: ${err.message ?? "Unknown error"}`);
                
                // Don't revert any other mods' installed status, just ensure this one is not installed
                setMods(prevMods => prevMods.map(m => 
                    m.id === modId ? { ...m, isInstalled: false } : m
                ));
                setError(`Failed to install ${modToInstall?.name ?? modId}: ${err.message ?? "Unknown error"}`);
            } finally {
                // Remove from installing set
                setInstallingModIds(prev => {
                    const next = new Set(prev);
                    next.delete(modId);
                    return next;
                });
            }
        }).catch(err => {
            console.error("Error in installation queue:", err);
            // Clean up in case of error
            setInstallingModIds(prev => {
                const next = new Set(prev);
                next.delete(modId);
                return next;
            });
        });
        
        // Return the promise so callers can await it if needed
        return installationQueue.current;
    };
    
    const handleHashWarningDecision = async (continueInstall: boolean) => {
        if (!pendingInstallAction) return;
        
        const { modId, hashVerificationFailed, loadingToastId } = pendingInstallAction;
        const modToInstall = mods.find(m => m.id === modId);
        
        if (!modToInstall) {
            setHashWarningVisible(false);
            setPendingInstallAction(null);
            return;
        }
        
        if (continueInstall) {
            // User chose to continue with installation despite failed hash
            toast.warning(`Installing ${modToInstall.name} despite failed hash verification`);
            
            // Update mod status with hash verification result
            setMods(prevMods => prevMods.map(prevMod =>
                prevMod.id === modId ? { 
                    ...prevMod, 
                    isInstalled: true,
                    hashVerified: !hashVerificationFailed
                } : prevMod
            ));
            
            // Add to installed mods reference
            installedModsRef.current.add(modId);
            
            // Update cache with the functional approach
            await saveCachedMods(prevMods => prevMods.map(prevMod =>
                prevMod.id === modId ? { 
                    ...prevMod, 
                    isInstalled: true,
                    hashVerified: !hashVerificationFailed
                } : prevMod
            ));
        } else {
            // User chose to abort installation
            toast.info(`Installation of ${modToInstall.name} was aborted`);
            
            // Revert the installed status
            setMods(prevMods => prevMods.map(m => m.id === modId ? { ...m, isInstalled: false } : m));
        }
        
        // Dismiss the loading toast properly by its ID
        if (loadingToastId) toast.dismiss(loadingToastId);
        
        // Close the warning modal and clear pending action
        setHashWarningVisible(false);
        setPendingInstallAction(null);
    };

    const uninstallMod = async (modId: string) => {
        setIsLoading(true);
        setError(null);
        let modToUninstall: Mod | undefined;

        try {
            modToUninstall = mods.find(m => m.id === modId);

            if (!modToUninstall) {
                throw new Error(`Mod '${modId}' not found.`);
            }

            for (const file of modToUninstall.downloadFiles) {
                await invoke("uninstall_mod", {
                    modId: modToUninstall.id,
                    downloadName: file.name
                });
            }

            // Update local state
            setMods(prevMods => 
                prevMods.map(prevMod =>
                    prevMod.id === modId ? { ...prevMod, isInstalled: false } : prevMod
                )
            );
            
            // Remove from installed mods set
            installedModsRef.current.delete(modId);

            // Update cache
            await saveCachedMods(prevMods => prevMods.map(prevMod =>
                prevMod.id === modId ? { ...prevMod, isInstalled: false } : prevMod
            ));
        } catch (err: any) {
            console.error(`Failed to uninstall mod ${modId}:`, err);
            setError(`Failed to uninstall ${modToUninstall?.name ?? modId}: ${err.message ?? "Unknown error"}`);
        } finally {
            setIsLoading(false);
        }
    };

    const checkForModUpdates = async (): Promise<ModUpdateInfo[]> => {
        setIsLoading(true);
        setError(null);
        const updates: ModUpdateInfo[] = [];

        try {
            const installedMods = mods.filter(mod => mod.isInstalled);
            if (installedMods.length === 0) {
                return [];
            }

            const response = await fetch(REPO_URL, { method: 'GET' });
            if (!response.ok) {
                throw new Error(`Failed to fetch mod list for update check: ${response.status}`);
            }
            const repoData = await response.json() as RepoMod[];
            const latestMods = convertRepoDataToMods(repoData);

            const latestModsMap = new Map(latestMods.map(mod => [mod.id, mod]));

            for (const installedMod of installedMods) {
                const latestMod = latestModsMap.get(installedMod.id);

                if (latestMod && latestMod.version !== installedMod.version) {
                    updates.push({
                        modId: installedMod.id,
                        currentVersion: installedMod.version,
                        latestVersion: latestMod.version,
                        downloadUrl: latestMod.downloadFiles[0].url,
                        changelogUrl: `${latestMod.repoUrl}/releases/tag/${latestMod.version}`
                    });
                }
            }

            console.log(`Found ${updates.length} mod updates available.`);
            return updates;
        } catch (err: any) {
            console.error("Failed to check for mod updates:", err);
            setError(`Failed to check for updates: ${err.message ?? "Unknown error"}`);
            return [];
        } finally {
            setIsLoading(false);
        }
    };

    const updateMod = async (modId: string): Promise<boolean> => {
        setIsLoading(true);
        setError(null);
        let updateInfo: ModUpdateInfo | undefined;
        let hashVerificationFailed = false;

        try {
            const availableUpdates = await checkForModUpdates();
            updateInfo = availableUpdates.find(update => update.modId === modId);

            if (!updateInfo) {
                console.log(`No update available or needed for mod ${modId}.`);
                return false;
            }

            const mod = mods.find(m => m.id === modId);
            if (!mod) {
                throw new Error(`Mod ${modId} not found in local state during update.`);
            }

            console.log(`Updating mod ${mod.name} from ${updateInfo.currentVersion} to ${updateInfo.latestVersion}`);

            await uninstallMod(modId);

            // Get latest mod info for hash verification
            const response = await fetch(REPO_URL, { method: 'GET' });
            if (!response.ok) {
                throw new Error(`Failed to fetch mod info: ${response.status}`);
            }
            
            const repoData = await response.json() as RepoMod[];
            const repoMod = repoData.find(rm => rm.full_name.toLowerCase().replace(/[^a-z0-9]/g, '-') === modId);
            
            if (!repoMod) {
                throw new Error(`Couldn't find mod ${modId} in repository for hash verification.`);
            }
            
            const results = [];
            // Use the download info from the repo which has the latest hash info
            for (const download of repoMod.downloads) {
                const result = await invoke("install_mod", {
                    modId: mod.id,
                    name: mod.name,
                    downloadUrl: download.url,
                    downloadName: download.name,
                    expectedHash: download.validHash || ""
                });
                
                // Cast the result to include the properties we expect
                const installResult = result as {
                    success: boolean;
                    hash_verified: boolean;
                    expected_hash: string;
                    actual_hash: string;
                    message: string;
                };
                
                results.push(installResult);
                
                // Only mark verification as failed if there was a hash to verify and it failed
                if (installResult.expected_hash && !installResult.hash_verified) {
                    hashVerificationFailed = true;
                }
            }
            
            // If hash verification failed, show warning but continue
            if (hashVerificationFailed) {
                const modName = mod.name;
                setTimeout(() => {
                    toast.warning(
                        <div>
                            <strong>Hash verification failed for updated mod {modName}</strong>
                            <p>The downloaded file doesn't match the expected hash. This may indicate the file has been tampered with.</p>
                            <p className="text-red-500 font-bold mt-1">Using this mod may pose a security risk!</p>
                        </div>,
                        { duration: 10000 }
                    );
                }, 500);
            }

            await saveCachedMods(prevMods => prevMods.map(prevMod =>
                prevMod.id === modId
                    ? { 
                        ...prevMod, 
                        version: updateInfo!.latestVersion, 
                        isInstalled: true,
                        hashVerified: !hashVerificationFailed
                    }
                    : prevMod
            ));

            console.log(`Mod ${mod.name} updated successfully to ${updateInfo.latestVersion}.`);
            return true;
        } catch (err: any) {
            console.error(`Failed to update mod ${modId}:`, err);
            setError(`Failed to update ${updateInfo?.modId ?? modId}: ${err.message ?? "Unknown error"}`);
            return false;
        } finally {
            setIsLoading(false);
        }
    };

    const getInstalledMods = () => mods.filter(mod => mod.isInstalled);

    const getAvailableMods = () => mods.filter(mod => !mod.isInstalled && !installingModIds.has(mod.id));

    const downloadModList = async () => {
        setIsLoading(true);
        setError(null);

        try {
            const modListData = {
                exportDate: new Date().toISOString(),
                mods: mods.map(mod => ({
                    id: mod.id,
                    name: mod.name,
                    author: mod.author,
                    version: mod.version,
                    description: mod.description,
                    isInstalled: mod.isInstalled,
                    downloadFiles: mod.downloadFiles,
                    repoUrl: mod.repoUrl
                }))
            };
            const modListJson = JSON.stringify(modListData, null, 2);

            const savePath = await save({
                filters: [{ name: 'JSON Files', extensions: ['json'] }],
                defaultPath: 'carbon-launcher-mod-list.json'
            });

            if (savePath) {
                await writeTextFile(savePath, modListJson);
                console.log(`Mod list saved to ${savePath}`);
            } else {
                console.log("File save cancelled by user.");
            }
        } catch (err: any) {
            console.error("Failed to export mod list:", err);
            if (err.message?.includes("dialog.save is not a function")) {
                setError("Could not open save dialog. This feature requires the Tauri environment.");
            } else {
                setError(`Failed to export mod list: ${err.message ?? "Unknown error"}`);
            }
        } finally {
            setIsLoading(false);
        }
    };

    const value = {
        mods,
        isLoading,
        error,
        refreshModList,
        installMod,
        uninstallMod,
        getInstalledMods,
        getAvailableMods,
        downloadModList,
        checkForModUpdates,
        updateMod,
        lastUpdated,
        installingModIds,
        isModInstalling,
    };

    return (
        <ModsContext.Provider value={value}>
            {children}
            {/* Render the hash verification warning dialog */}
            <HashVerificationWarning
                open={hashWarningVisible}
                onClose={() => handleHashWarningDecision(false)}
                modName={hashWarningModName}
                onContinue={() => handleHashWarningDecision(true)}
                onAbort={() => handleHashWarningDecision(false)}
            />
        </ModsContext.Provider>
    );
}

export const useModsContext = () => {
    const context = useContext(ModsContext);
    if (context === undefined) {
        throw new Error("useModsContext must be used within a ModsProvider");
    }
    return context;
};