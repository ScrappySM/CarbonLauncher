use tauri::AppHandle;
use tauri::Manager;

use tauri_plugin_http::reqwest;
use std::path::PathBuf;
use std::io::Write;
use std::time::Duration;
use std::sync::Mutex;
use sha2::{Sha256, Digest};

use std::{ffi::CString, mem::size_of, ptr::null_mut};
use windows::{
    core::{self, PCSTR},
    Win32::Foundation::*,
    Win32::System::Diagnostics::ToolHelp::*,
    Win32::System::LibraryLoader::GetModuleHandleA,
    Win32::System::Memory::*,
    Win32::System::Threading::*
};

// Global rate limiter for image requests
struct RateLimiter {
    last_request: std::time::Instant,
    delay_ms: u64,
}

lazy_static::lazy_static! {
    static ref RATE_LIMITER: Mutex<RateLimiter> = Mutex::new(RateLimiter {
        last_request: std::time::Instant::now(),
        delay_ms: 200, // 200ms between requests to prevent rate limiting
    });
}

#[derive(serde::Serialize)]
struct ModInstallResult {
    success: bool,
    hash_verified: bool,
    expected_hash: String,
    actual_hash: String,
    message: String,
}

// New struct to return image caching results
#[derive(serde::Serialize)]
struct ImageResult {
    cached: bool,
    path: String,
}

// Function to get or cache an image
#[tauri::command]
async fn get_or_cache_image(app_handle: AppHandle, image_url: String, mod_id: String) -> Result<ImageResult, String> {
    println!("Caching image for mod: {}", mod_id);
    
    // Create a unique filename from the URL
    let url_hash = {
        let mut hasher = Sha256::new();
        hasher.update(image_url.as_bytes());
        format!("{:x}", hasher.finalize())
    };
    
    // Determine file extension from URL or default to .png
    let extension = match image_url.split('.').last() {
        Some(ext) if !ext.contains('/') && !ext.contains('?') => ext,
        _ => "png", // Default to png if we can't determine the extension
    };
    let filename = format!("{}.{}", url_hash, extension);
    
    // Get the cache directory
    let path_resolver = app_handle.path();
    let app_data_path = path_resolver.app_data_dir().map_err(|e| {
        println!("Failed to get app data dir: {}", e);
        e.to_string()
    })?;
    
    // First ensure app data directory exists
    if !app_data_path.exists() {
        println!("App data directory doesn't exist, creating: {:?}", app_data_path);
        match std::fs::create_dir_all(&app_data_path) {
            Ok(_) => println!("App data directory created successfully"),
            Err(e) => {
                let err_msg = format!("Failed to create app data directory: {}", e);
                println!("{}", err_msg);
                return Err(err_msg);
            }
        }
    }
    
    // Create image cache directory
    let cache_dir = app_data_path.join("image-cache");
    println!("Image cache directory path: {:?}", cache_dir);
    
    // Create cache directory if it doesn't exist
    if !cache_dir.exists() {
        println!("Creating image cache directory: {:?}", cache_dir);
        match std::fs::create_dir_all(&cache_dir) {
            Ok(_) => println!("Image cache directory created successfully"),
            Err(e) => {
                let err_msg = format!("Failed to create image cache directory: {}", e);
                println!("{}", err_msg);
                return Err(err_msg);
            }
        }
    }
    
    // Create mod-specific sub-directory (keeps images organized by mod)
    let mod_cache_dir = cache_dir.join(&mod_id);
    println!("Mod cache directory: {:?}", mod_cache_dir);
    
    if !mod_cache_dir.exists() {
        println!("Creating mod cache directory: {:?}", mod_cache_dir);
        match std::fs::create_dir_all(&mod_cache_dir) {
            Ok(_) => println!("Mod cache directory created successfully"),
            Err(e) => {
                let err_msg = format!("Failed to create mod cache directory: {}", e);
                println!("{}", err_msg);
                return Err(err_msg);
            }
        }
    }
    
    let image_path = mod_cache_dir.join(&filename);
    
    // Convert to string in a reliable way
    let image_path_str = match image_path.to_str() {
        Some(s) => s.to_string(),
        None => return Err("Failed to convert image path to string".to_string()),
    };
    
    println!("Final image path: {:?}", image_path);
    
    // If image already exists in cache, return the path immediately
    if image_path.exists() {
        println!("Image found in cache");
        return Ok(ImageResult {
            cached: true,
            // Don't add any protocol prefix here, this will be handled by convertFileSrc in the frontend
            path: image_path_str,
        });
    }
    
    // Rate limit image requests to prevent 429 errors
    {
        let mut limiter = RATE_LIMITER.lock().unwrap();
        let elapsed = limiter.last_request.elapsed();
        if elapsed < Duration::from_millis(limiter.delay_ms) {
            let sleep_time = Duration::from_millis(limiter.delay_ms) - elapsed;
            std::thread::sleep(sleep_time);
        }
        limiter.last_request = std::time::Instant::now();
    }
    
    println!("Downloading image from: {}", image_url);
    
    // Download the image
    let response = reqwest::get(&image_url).await.map_err(|e| {
        println!("Failed to download image: {}", e);
        e.to_string()
    })?;
    
    // Check status code - if it's a rate limit (429), suggest a longer delay
    if response.status() == reqwest::StatusCode::TOO_MANY_REQUESTS {
        // Increase the delay for future requests
        let mut limiter = RATE_LIMITER.lock().unwrap();
        limiter.delay_ms = limiter.delay_ms.saturating_mul(2);
        return Err("Rate limited by server. Try again later.".to_string());
    }
    
    if !response.status().is_success() {
        let err_msg = format!("Failed to download image: HTTP {}", response.status());
        println!("{}", err_msg);
        return Err(err_msg);
    }
    
    // Get image bytes
    let bytes = response.bytes().await.map_err(|e| {
        let err_msg = format!("Failed to read image bytes: {}", e);
        println!("{}", err_msg);
        err_msg
    })?;
    
    println!("Downloaded image, size: {} bytes", bytes.len());
    
    // Write to cache with more detailed error handling
    println!("Creating image file at: {:?}", image_path);
    match std::fs::File::create(&image_path) {
        Ok(mut file) => {
            println!("Image file created successfully, writing data");
            match file.write_all(&bytes) {
                Ok(_) => println!("Image data written successfully"),
                Err(e) => {
                    let err_msg = format!("Failed to write image to cache: {}", e);
                    println!("{}", err_msg);
                    return Err(err_msg);
                }
            }
        },
        Err(e) => {
            let err_msg = format!("Failed to create image file: {} at path: {:?}", e, image_path);
            println!("{}", err_msg);
            return Err(err_msg);
        }
    }
    
    println!("Image successfully downloaded and cached");
    
    Ok(ImageResult {
        cached: false,
        // Don't add any protocol prefix here, this will be handled by convertFileSrc in the frontend
        path: image_path_str,
    })
}

// Function to clear the image cache
#[tauri::command]
fn clear_image_cache(app_handle: AppHandle) -> Result<String, String> {
    let path_resolver = app_handle.path();
    let app_data_path = path_resolver.app_data_dir().map_err(|e| e.to_string())?;
    let cache_dir = app_data_path.join("image-cache");
    
    if cache_dir.exists() {
        std::fs::remove_dir_all(&cache_dir).map_err(|e| e.to_string())?;
        std::fs::create_dir_all(&cache_dir).map_err(|e| e.to_string())?;
    }
    
    Ok("Image cache cleared successfully.".to_string())
}

#[tauri::command]
async fn install_mod(app_handle: AppHandle, mod_id: String, name: String, download_url: String, download_name: String, expected_hash: String) -> Result<ModInstallResult, String> {
    println!("Installing mod: {} (ID: {})", name, mod_id);
    println!("Expected hash: {}", expected_hash);

    let path_resolver = app_handle.path();
    let app_data_path = path_resolver.app_data_dir().map_err(|e| e.to_string())?;

    // We need to create the mod directory if it doesn't exist
    let mod_dir = app_data_path.join("mods").join(&mod_id);
    if !mod_dir.exists() {
        std::fs::create_dir_all(&mod_dir).map_err(|e| e.to_string())?;
    }

    // Download the mod file
    let response = reqwest::get(&download_url).await.map_err(|e| e.to_string())?;
    if !response.status().is_success() {
        return Err(format!("Failed to download mod: {}", response.status()));
    }

    let bytes = response.bytes().await.map_err(|e| e.to_string())?;
    
    // Verify hash before writing to disk
    let mut hasher = Sha256::new();
    hasher.update(&bytes);
    let actual_hash = format!("{:x}", hasher.finalize());
    let hash_verified = !expected_hash.is_empty() && expected_hash == actual_hash;
    
    println!("Actual hash: {}", actual_hash);
    println!("Hash verification: {}", hash_verified);

    // Write file regardless of hash verification (let the UI decide what to do)
    let mod_file_path = mod_dir.join(&download_name);
    let mut file = std::fs::File::create(&mod_file_path).map_err(|e| e.to_string())?;
    file.write_all(&bytes).map_err(|e| e.to_string())?;

    let message = if hash_verified || expected_hash.is_empty() {
        format!("Mod {} installed successfully.", name)
    } else {
        format!("Mod {} installed, but hash verification failed!", name)
    };

    Ok(ModInstallResult {
        success: true,
        hash_verified,
        expected_hash,
        actual_hash,
        message,
    })
}

#[tauri::command]
fn uninstall_mod(app_handle: AppHandle, mod_id: String) -> Result<String, String> {
    println!("Uninstalling mod: {}", mod_id);

    let path_resolver = app_handle.path();
    let app_data_path = path_resolver.app_data_dir().map_err(|e| e.to_string())?;

    println!("App data path: {:?}", app_data_path);

    let dir = app_data_path.join("mods").join(&mod_id);
    if !dir.exists() {
        return Err(format!("Mod {} not found.", mod_id));
    }

    println!("Deleting mod directory: {:?}", dir);
    std::fs::remove_dir_all(&dir).map_err(|e| e.to_string())?;
    
    Ok(format!("Mod {} uninstalled successfully.", mod_id))
}

fn find_process_id(target_process: &str) -> Option<u32> {
    unsafe {
        let snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0).unwrap();
        let mut entry = PROCESSENTRY32 {
            dwSize: size_of::<PROCESSENTRY32>() as u32,
            ..Default::default()
        };

        if Process32First(snapshot, &mut entry).is_ok() {
            loop {
                let process_name_bytes = entry.szExeFile.iter().take_while(|&&c| c != 0).map(|&c| c as u8).collect::<Vec<u8>>();
                let exe_name = String::from_utf8_lossy(&process_name_bytes);

                if exe_name.eq_ignore_ascii_case(target_process) {
                    CloseHandle(snapshot);
                    return Some(entry.th32ProcessID);
                }

                if !Process32Next(snapshot, &mut entry).is_ok() {
                    break;
                }
            }
        }

        CloseHandle(snapshot);
    }
    None
}

fn open_process(process_id: u32) -> Option<HANDLE> {
    unsafe {
        let process_handle = OpenProcess(PROCESS_ALL_ACCESS, false, process_id);
        if process_handle.is_err() {
            return None;
        }

        let process_handle = process_handle.unwrap();
        Some(process_handle)
    }
}

#[tauri::command]
async fn start_game() -> Result<String, String> {
    println!("Starting game...");
    
    #[cfg(target_os = "windows")]
    {
        let steam_url = "steam://run/387990";
        std::process::Command::new("cmd")
            .arg("/C")
            .arg("start")
            .arg(steam_url)
            .spawn()
            .map_err(|e| format!("Failed to start game: {}", e))?;
    }
    
    let mut process_id = find_process_id("ScrapMechanic.exe");
    while process_id.is_none() {
        println!("Waiting for Scrap Mechanic to start...");
        std::thread::sleep(Duration::from_secs(1));
        process_id = find_process_id("ScrapMechanic.exe");
    }

    let process_id = process_id.unwrap(); // pray
    println!("Found Scrap Mechanic process with ID: {}", process_id);

    let process_handle = open_process(process_id).ok_or("Failed to open process")?;
    println!("Opened process handle: {:?}", process_handle);

    Ok("Game started successfully.".to_string())
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_fs::init())
        .plugin(tauri_plugin_http::init())
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![install_mod, uninstall_mod, start_game, get_or_cache_image, clear_image_cache])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
