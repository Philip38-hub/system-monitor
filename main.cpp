#include "header.h"
#include <SDL.h>
#include <set>
#include <unordered_map> // For std::unordered_map
#include <map> // For std::map

/*
NOTE : You are free to change the code as you wish, the main objective is to make the
       application work and pass the audit.

       It will be provided the main function with the following functions :

       - `void systemWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the system window on your screen
       - `void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the memory and processes window on your screen
       - `void networkWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the network window on your screen
*/

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h> // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h> // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h> // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE      // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h> // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE        // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h> // Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

static HistoryData cpu_history;
static HistoryData fan_history;
static HistoryData thermal_history;
static bool plot_paused = false;
static float history_fps = 60.0f;
static float network_max_usage_gb = 2.0f; // Default max usage for network visualization in GB

// systemWindow, display information for the system monitorization
void systemWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // student TODO : add code here for the system window
    ImGui::Text("Operating System: %s", getOsName());
    ImGui::Text("Logged in User: %s", getLoggedInUser().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());
    ImGui::Text("Total Processes: %d", getTotalProcesses());
    ImGui::Text("CPU Type: %s", CPUinfo().c_str());

    static float history_scale = 1.0f;

    ImGui::Checkbox("Pause Plot", &plot_paused);
    ImGui::SliderFloat("Plot FPS", &history_fps, 1.0f, 120.0f, "%.0f FPS");
    ImGui::SliderFloat("Plot Y-Scale", &history_scale, 0.1f, 2.0f, "%.1f");

    if (ImGui::BeginTabBar("SystemTabs"))
    {
        if (ImGui::BeginTabItem("CPU"))
        {
            // student TODO: CPU graph and overlay
            ImGui::PlotLines("##CPU", cpu_history.values.data(), cpu_history.values.size(), cpu_history.offset,
                             cpu_history.overlay_text.c_str(), 0.0f, 100.0f * history_scale, ImVec2(0, ImGui::GetContentRegionAvail().y));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Fan"))
        {
            // student TODO: Fan information and graph
            ImGui::Text("Status: %s", getFanStatus().c_str());
            ImGui::Text("Speed: %.0f RPM", getFanSpeed());
            ImGui::PlotLines("##Fan", fan_history.values.data(), fan_history.values.size(), fan_history.offset,
                             fan_history.overlay_text.c_str(), 0.0f, fan_history.max_value * history_scale, ImVec2(0, ImGui::GetContentRegionAvail().y));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Thermal"))
        {
            // student TODO: Thermal information and graph
            ImGui::Text("Temperature: %.1f C", getCPUTemperature());
            ImGui::PlotLines("##Thermal", thermal_history.values.data(), thermal_history.values.size(), thermal_history.offset,
                             thermal_history.overlay_text.c_str(), 0.0f, thermal_history.max_value * history_scale, ImVec2(0, ImGui::GetContentRegionAvail().y));
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

// Structure to store process CPU usage tracking data
struct ProcessCPUData {
    long long lastProcTime = 0;
    long long lastTotalTime = 0;
    float lastCpuUsage = 0.0f;
    float lastUpdateTime = 0.0f;
    static constexpr float updateInterval = 0.5f; // Update interval in seconds
};

// Map to store process CPU usage data
static std::unordered_map<int, ProcessCPUData> processCpuData;

// Helper to get the number of CPU cores
static int getNumCores() {
    static int num_cores = 0;
    if (num_cores == 0) {
        num_cores = sysconf(_SC_NPROCESSORS_ONLN);
        if (num_cores <= 0) num_cores = 1; // Fallback to 1 core if detection fails
    }
    return num_cores;
}

// Helper to calculate CPU usage for a process
float calculateProcessCPUUsage(const Proc &p, const Proc &prev_p, const CPUStats &prev_cpu, const CPUStats &current_cpu)
{
    float currentTime = ImGui::GetTime();
    
    // Get or create process CPU data
    ProcessCPUData &cpuData = processCpuData[p.pid];
    
    // Return cached value if not time to update
    if (currentTime - cpuData.lastUpdateTime < ProcessCPUData::updateInterval) {
        return cpuData.lastCpuUsage;
    }
    
    // Get number of CPU cores for normalization
    static int numCores = getNumCores();
    
    // Get the system's clock ticks per second
    static long hz = sysconf(_SC_CLK_TCK);
    if (hz <= 0) hz = 100; // Fallback value if can't determine
    
    // Calculate process CPU time delta
    long long processCPUTimeDelta = (p.utime + p.stime) - cpuData.lastProcTime;
    
    // Calculate total system CPU time delta
    long long totalSystemTimeDelta = (current_cpu.user - prev_cpu.user) +
                                     (current_cpu.nice - prev_cpu.nice) +
                                     (current_cpu.system - prev_cpu.system) +
                                     (current_cpu.idle - prev_cpu.idle) +
                                     (current_cpu.iowait - prev_cpu.iowait) +
                                     (current_cpu.irq - prev_cpu.irq) +
                                     (current_cpu.softirq - prev_cpu.softirq) +
                                     (current_cpu.steal - prev_cpu.steal);
 
    float cpuUsage = 0.0f;
    
    if (totalSystemTimeDelta > 0 && processCPUTimeDelta >= 0) {
        // Calculate the time interval in seconds
        float timeInterval = (currentTime - cpuData.lastUpdateTime);
        
        // Convert process CPU time delta from jiffies to seconds
        float processTime = (float)processCPUTimeDelta / (float)hz;
        
        // Calculate CPU usage percentage
        if (timeInterval > 0) {
            // This formula should more closely match top's calculation
            // It represents the fraction of CPU time the process used during the interval.
            cpuUsage = (processTime / timeInterval) * 100.0f;
            
            // top displays per-core usage, so a single process can use up to 100% * num_cores.
            // However, if we are reporting the total CPU usage of a process across all cores,
            // then capping at 100% might be appropriate depending on the desired output.
            // For now, let's keep the cap at 100% as it was before, assuming we want the percentage of *one* core's capacity.
            // If the goal is to match top's total usage across all cores, this might need adjustment.
            cpuUsage = std::min(cpuUsage, 100.0f);
            
            // Ensure we don't show negative values
            cpuUsage = std::max(0.0f, cpuUsage);
            
            // Debug output (uncomment to see values)
            // printf("PID %d: procTimeDelta=%lld, totalSystemTimeDelta=%lld, interval=%.2fs, raw=%.2f%%, cpu=%.2f%%\n",
            //        p.pid, processCPUTimeDelta, totalSystemTimeDelta, timeInterval,
            //        (processTime / timeInterval) * 100.0f, cpuUsage);
        }
    }
    
    // Update stored values
    cpuData.lastProcTime = p.utime + p.stime; // Store the current total process time
    // cpuData.lastTotalTime = currentTotalSystemTime; // This line is removed as currentTotalSystemTime is not used
    cpuData.lastCpuUsage = cpuUsage;
    cpuData.lastUpdateTime = currentTime;
    
    return cpuUsage;
}

// Helper to calculate memory usage for a process
float calculateProcessMemoryUsage(const Proc &p, long long totalRam)
{
    if (totalRam == 0)
        return 0.0f;
    return (float)(p.rss * 1024) / (float)totalRam * 100.0f;
}

// memoryProcessesWindow, display information for the memory and processes information
void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // student TODO : add code here for the memory and process information
    ImGui::Text("Physical Memory (RAM) Usage: %.1f%%", getMemoryUsage());
    ImGui::ProgressBar(getMemoryUsage() / 100.0f, ImVec2(0.0f, 0.0f));

    ImGui::Text("Virtual Memory (SWAP) Usage: %.1f%%", getSwapUsage());
    ImGui::ProgressBar(getSwapUsage() / 100.0f, ImVec2(0.0f, 0.0f));

    ImGui::Text("Disk Usage: %.1f%%", getDiskUsage());
    ImGui::ProgressBar(getDiskUsage() / 100.0f, ImVec2(0.0f, 0.0f));

    static ImGuiTextFilter filter;
    filter.Draw("Filter processes", 300);

    ImGui::Separator();

    if (ImGui::BeginTabBar("MemoryProcessesTabs"))
    {
        if (ImGui::BeginTabItem("Processes"))
        {
            static vector<Proc> processes;
            static std::map<int, float> process_cpu_usage;
            static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ScrollY;
            static std::set<int> selected_pids;

            // Track previous CPU stats for delta calculation
            static std::map<int, Proc> prev_proc_stats;
            static bool first_run = true;
            static float last_update_time = 0.0f;

            static CPUStats prev_cpu_stats = {};

            float current_time = ImGui::GetTime();
            if (current_time - last_update_time > 1.0f)
            {
                last_update_time = current_time;

                CPUStats current_cpu_stats = getCPUStats();
                vector<Proc> current_processes = getAllProcesses();

                if (!first_run)
                {
                    for (const auto &p : current_processes)
                    {
                        if (prev_proc_stats.count(p.pid) > 0)
                        {
                            process_cpu_usage[p.pid] = calculateProcessCPUUsage(p, prev_proc_stats.at(p.pid), prev_cpu_stats, current_cpu_stats);
                        }
                    }
                }

                prev_proc_stats.clear();
                for (const auto &p : current_processes)
                {
                    prev_proc_stats[p.pid] = p;
                }
                processes = current_processes;
                prev_cpu_stats = current_cpu_stats;
                first_run = false;
            }

            if (ImGui::BeginTable("ProcessesTable", 6, flags, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 15)))
            {
                ImGui::TableSetupColumn("Select");
                ImGui::TableSetupColumn("PID");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("State");
                ImGui::TableSetupColumn("CPU Usage (%)");
                ImGui::TableSetupColumn("Memory Usage (%)");
                ImGui::TableHeadersRow();

                // Get total RAM for memory usage calculation
                struct sysinfo memInfo;
                sysinfo(&memInfo);
                long long totalRam = memInfo.totalram * memInfo.mem_unit;

                for (const auto &p : processes)
                {
                    if (!filter.PassFilter(p.name.c_str()))
                        continue;

                    ImGui::TableNextRow();

                    // Multi-row selection checkbox
                    ImGui::TableNextColumn();
                    bool selected = selected_pids.count(p.pid) > 0;
                    if (ImGui::Checkbox(("##select_" + std::to_string(p.pid)).c_str(), &selected))
                    {
                        if (selected)
                            selected_pids.insert(p.pid);
                        else
                            selected_pids.erase(p.pid);
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", p.pid);
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", p.name.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%c", p.state);
                    ImGui::TableNextColumn();

                    // Display the stored CPU usage
                    ImGui::Text("%.2f", process_cpu_usage.count(p.pid) ? process_cpu_usage[p.pid] : 0.0f);

                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", calculateProcessMemoryUsage(p, totalRam));
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

// network, display information network information
void networkWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Network Interfaces section
    ImGui::Text("Network Interfaces");
    ImGui::Separator();

    if (ImGui::BeginTable("NetworkInterfaces", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Interface", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("IPv4 Address");
        ImGui::TableHeadersRow();

        vector<IP4> ip4s = getIPv4Addresses();
        for (const auto &ip : ip4s)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", ip.name);
            ImGui::TableNextColumn();
            ImGui::Text("%s", ip.addressBuffer);
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SliderFloat("Max Usage (GB)", &network_max_usage_gb, 0.1f, 10.0f, "%.1f GB"); // Slider for max usage in GB

    if (ImGui::BeginTabBar("NetworkTabs"))
    {
        if (ImGui::BeginTabItem("Visuals"))
        {
            if (ImGui::BeginTabBar("VisualsTabs"))
            {
                if (ImGui::BeginTabItem("RX"))
                {
                    map<string, RX> rxStats = getRXStats();
                    ImGui::Text("RX Usage:");
                    ImGui::Separator();
                    for (const auto &[interface, rx] : rxStats)
                    {
                        ImGui::Text("%s RX: %s", interface.c_str(), formatBytes(rx.bytes).c_str());
                        ImGui::ProgressBar((float)rx.bytes / (network_max_usage_gb * 1024.0f * 1024.0f * 1024.0f), ImVec2(0.0f, 0.0f));
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("TX"))
                {
                    map<string, TX> txStats = getTXStats();
                    ImGui::Text("TX Usage:");
                    ImGui::Separator();
                    for (const auto &[interface, tx] : txStats)
                    {
                        ImGui::Text("%s TX: %s", interface.c_str(), formatBytes(tx.bytes).c_str());
                        ImGui::ProgressBar((float)tx.bytes / (network_max_usage_gb * 1024.0f * 1024.0f * 1024.0f), ImVec2(0.0f, 0.0f));
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Statistics"))
        {
            if (ImGui::CollapsingHeader("RX Statistics"))
            {
                if (ImGui::BeginTable("RXTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
                {
                    ImGui::TableSetupColumn("Interface");
                    ImGui::TableSetupColumn("Bytes");
                    ImGui::TableSetupColumn("Packets");
                    ImGui::TableSetupColumn("Errors");
                    ImGui::TableSetupColumn("Drop");
                    ImGui::TableSetupColumn("FIFO");
                    ImGui::TableSetupColumn("Collisions");
                    ImGui::TableSetupColumn("Carrier");
                    ImGui::TableHeadersRow();

                    map<string, RX> rxStats = getRXStats();
                    for (const auto &[interface, rx] : rxStats)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%s", interface.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", formatBytes(rx.bytes).c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%d", rx.packets);
                        ImGui::TableNextColumn(); ImGui::Text("%d", rx.errs);
                        ImGui::TableNextColumn(); ImGui::Text("%d", rx.drop);
                        ImGui::TableNextColumn(); ImGui::Text("%d", rx.fifo);
                        ImGui::TableNextColumn(); ImGui::Text("%d", rx.colls);
                        ImGui::TableNextColumn(); ImGui::Text("%d", rx.carrier);
                    }
                    ImGui::EndTable();
                }
            }

            if (ImGui::CollapsingHeader("TX Statistics"))
            {
                if (ImGui::BeginTable("TXTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
                {
                    ImGui::TableSetupColumn("Interface");
                    ImGui::TableSetupColumn("Bytes");
                    ImGui::TableSetupColumn("Packets");
                    ImGui::TableSetupColumn("Errors");
                    ImGui::TableSetupColumn("Drop");
                    ImGui::TableSetupColumn("FIFO");
                    ImGui::TableSetupColumn("Frame");
                    ImGui::TableSetupColumn("Compressed");
                    ImGui::TableHeadersRow();

                    map<string, TX> txStats = getTXStats();
                    for (const auto &[interface, tx] : txStats)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%s", interface.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", formatBytes(tx.bytes).c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%d", tx.packets);
                        ImGui::TableNextColumn(); ImGui::Text("%d", tx.errs);
                        ImGui::TableNextColumn(); ImGui::Text("%d", tx.drop);
                        ImGui::TableNextColumn(); ImGui::Text("%d", tx.fifo);
                        ImGui::TableNextColumn(); ImGui::Text("%d", tx.frame);
                        ImGui::TableNextColumn(); ImGui::Text("%d", tx.compressed);
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}


// Main code
int main(int, char **)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char *name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // render bindings
    ImGuiIO &io = ImGui::GetIO();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // background color
    // note : you are free to change the style of the application
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // History data for plots
    cpu_history.color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for CPU
    fan_history.color = ImVec4(0.0f, 0.5f, 1.0f, 1.0f); // Blue for Fan
    thermal_history.color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for Thermal

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        {
            ImVec2 mainDisplay = io.DisplaySize;
            memoryProcessesWindow("== Memory and Processes ==",
                                  ImVec2((mainDisplay.x / 2) - 20, (mainDisplay.y / 2) + 30),
                                  ImVec2((mainDisplay.x / 2) + 10, 10));
            // --------------------------------------
            systemWindow("== System ==",
                         ImVec2((mainDisplay.x / 2) - 10, (mainDisplay.y / 2) + 30),
                         ImVec2(10, 10));
            // --------------------------------------
            networkWindow("== Network ==",
                          ImVec2(mainDisplay.x - 20, (mainDisplay.y / 2) - 60),
                          ImVec2(10, (mainDisplay.y / 2) + 50));
        }

        // Update history data
       static float last_update_time = 0.0f;
       float current_time = ImGui::GetTime();
       if (!plot_paused && (current_time - last_update_time) > (1.0f/history_fps))
       {
            last_update_time = current_time;

            cpu_history.addValue(getCPUUsage());
            fan_history.addValue(getFanSpeed());
            thermal_history.addValue(getCPUTemperature());
       }
        char buffer[64];
        
        if (!cpu_history.values.empty()) {
            snprintf(buffer, sizeof(buffer), "%.1f %%", cpu_history.values[cpu_history.offset == 0 ? cpu_history.values.size() - 1 : cpu_history.offset - 1]);
            cpu_history.overlay_text = buffer;
        }

        if (!fan_history.values.empty()) {
            snprintf(buffer, sizeof(buffer), "%.0f RPM", fan_history.values[fan_history.offset == 0 ? fan_history.values.size() - 1 : fan_history.offset - 1]);
            fan_history.overlay_text = buffer;
        }

        if (!thermal_history.values.empty()) {
            snprintf(buffer, sizeof(buffer), "%.1f C", thermal_history.values[thermal_history.offset == 0 ? thermal_history.values.size() - 1 : thermal_history.offset - 1]);
            thermal_history.overlay_text = buffer;
        }



        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
