using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;

internal static class Program
{
    [STAThread]
    static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new LauncherForm());
    }
}

public sealed class LauncherForm : Form
{
    const string ApiUrl = "https://api.github.com/repos/HyperlinksSpace/threedensity/releases/latest";
    const string FallbackZip = "https://github.com/HyperlinksSpace/threedensity/releases/latest/download/ThreeDensity-Win64.zip";
    const string FallbackSetup = "https://github.com/HyperlinksSpace/threedensity/releases/latest/download/ThreeDensitySetup.exe";
    const string LauncherFileName = "ThreeDensityLauncher.exe";

    readonly PictureBox logo;
    readonly Label title;
    readonly Label subtitle;
    readonly Label status;
    readonly ProgressBar progress;
    readonly Button playOffline;
    readonly Button retry;

    readonly string installRoot;
    readonly string gameDir;
    readonly string versionFile;
    readonly string launcherPath;
    string launchPath;
    bool busy;

    public LauncherForm()
    {
        Text = "Three Density";
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(540, 360);
        BackColor = Color.FromArgb(12, 12, 11);
        ForeColor = Color.FromArgb(230, 224, 214);
        Font = new Font("Segoe UI", 10f);

        try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

        installRoot = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ThreeDensity");
        gameDir = Path.Combine(installRoot, "Game");
        versionFile = Path.Combine(installRoot, "version.txt");
        launcherPath = Path.Combine(installRoot, LauncherFileName);

        logo = new PictureBox
        {
            Bounds = new Rectangle(24, 18, 72, 72),
            SizeMode = PictureBoxSizeMode.Zoom,
            BackColor = Color.Transparent
        };
        try { logo.Image = LogoData.LoadMark(); } catch { }

        title = new Label
        {
            AutoSize = false,
            Bounds = new Rectangle(110, 28, 400, 36),
            Font = new Font("Segoe UI", 22f, FontStyle.Bold),
            ForeColor = Color.FromArgb(243, 238, 230),
            Text = "THREE DENSITY"
        };
        subtitle = new Label
        {
            AutoSize = false,
            Bounds = new Rectangle(110, 66, 400, 40),
            ForeColor = Color.FromArgb(170, 166, 158),
            Text = "Launcher checks GitHub for updates,\nthen starts the game."
        };
        status = new Label
        {
            AutoSize = false,
            Bounds = new Rectangle(28, 150, 480, 48),
            Text = "Starting…"
        };
        progress = new ProgressBar
        {
            Bounds = new Rectangle(28, 210, 484, 18),
            Style = ProgressBarStyle.Continuous
        };
        playOffline = new Button
        {
            Bounds = new Rectangle(28, 270, 170, 40),
            FlatStyle = FlatStyle.Flat,
            BackColor = Color.FromArgb(40, 40, 38),
            ForeColor = Color.White,
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            Text = "Play offline",
            Enabled = false,
            Visible = false
        };
        playOffline.FlatAppearance.BorderSize = 0;
        playOffline.Click += (s, e) => LaunchGameAndExit();

        retry = new Button
        {
            Bounds = new Rectangle(214, 270, 120, 40),
            FlatStyle = FlatStyle.Flat,
            BackColor = Color.FromArgb(196, 92, 28),
            ForeColor = Color.White,
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            Text = "Retry",
            Visible = false
        };
        retry.FlatAppearance.BorderSize = 0;
        retry.Click += (s, e) => BeginBootstrap();

        Controls.Add(logo);
        Controls.Add(title);
        Controls.Add(subtitle);
        Controls.Add(status);
        Controls.Add(progress);
        Controls.Add(playOffline);
        Controls.Add(retry);

        Shown += (s, e) => BeginBootstrap();
    }

    void BeginBootstrap()
    {
        if (busy) return;
        busy = true;
        playOffline.Visible = false;
        retry.Visible = false;
        progress.Value = 0;

        ThreadPool.QueueUserWorkItem(state =>
        {
            try
            {
                ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
                Directory.CreateDirectory(installRoot);

                SetStatus("Preparing launcher…", 4);
                EnsureInstalledLauncherAndShortcuts();

                SetStatus("Checking GitHub for updates…", 10);
                ReleaseInfo release = FetchLatestRelease();
                string installed = ReadInstalledVersion();
                launchPath = FindExe(gameDir);

                bool needsInstall = string.IsNullOrEmpty(launchPath) || !File.Exists(launchPath);
                bool needsUpdate = needsInstall || !VersionsEqual(installed, release.Tag);

                if (needsUpdate)
                {
                    if (needsInstall)
                    {
                        SetStatus("Downloading Three Density " + release.Tag + "…", 15);
                    }
                    else
                    {
                        SetStatus("Update found: " + release.Tag + " (have " + installed + "). Downloading…", 15);
                    }

                    string zipUrl = string.IsNullOrEmpty(release.ZipUrl) ? FallbackZip : release.ZipUrl;
                    string zipPath = Path.Combine(installRoot, "ThreeDensity-Win64.zip");
                    Download(zipUrl, zipPath);

                    SetStatus("Installing " + release.Tag + "…", 82);
                    InstallGameFromZip(zipPath);
                    try { File.Delete(zipPath); } catch { }

                    launchPath = FindExe(gameDir);
                    if (string.IsNullOrEmpty(launchPath) || !File.Exists(launchPath))
                    {
                        throw new InvalidOperationException("Install finished but threedensity.exe was not found.");
                    }

                    File.WriteAllText(versionFile, release.Tag ?? "");
                    TryRefreshLauncher(release.SetupUrl);
                    EnsureInstalledLauncherAndShortcuts();
                    SetStatus("Updated to " + release.Tag + ". Launching…", 96);
                }
                else
                {
                    SetStatus("Up to date (" + installed + "). Launching…", 90);
                }

                Thread.Sleep(400);
                Invoke(new Action(LaunchGameAndExit));
            }
            catch (Exception ex)
            {
                launchPath = FindExe(gameDir);
                bool canOffline = !string.IsNullOrEmpty(launchPath) && File.Exists(launchPath);
                SetStatus("Update check failed: " + ex.Message + (canOffline ? "\nYou can still play the installed build." : ""), 0);
                Invoke(new Action(() =>
                {
                    busy = false;
                    retry.Visible = true;
                    playOffline.Visible = canOffline;
                    playOffline.Enabled = canOffline;
                }));
            }
        });
    }

    void EnsureInstalledLauncherAndShortcuts()
    {
        string current = Application.ExecutablePath;
        try
        {
            if (!PathsEqual(current, launcherPath))
            {
                File.Copy(current, launcherPath, true);
            }
        }
        catch
        {
            // Still create shortcuts to whatever we are running.
        }

        string target = File.Exists(launcherPath) ? launcherPath : current;
        CreateShortcut(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "Three Density.lnk"), target);
        string startMenu = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.StartMenu), "Programs");
        Directory.CreateDirectory(startMenu);
        CreateShortcut(Path.Combine(startMenu, "Three Density.lnk"), target);
    }

    void TryRefreshLauncher(string setupUrl)
    {
        if (string.IsNullOrEmpty(setupUrl)) return;
        try
        {
            string temp = Path.Combine(installRoot, "ThreeDensitySetup.download.exe");
            DownloadQuiet(setupUrl, temp);
            string pending = Path.Combine(installRoot, "ThreeDensityLauncher.pending.exe");
            if (File.Exists(pending)) try { File.Delete(pending); } catch { }
            File.Move(temp, pending);

            // Replace launcher after this process exits.
            string bat = Path.Combine(installRoot, "update-launcher.bat");
            string script =
                "@echo off\r\n" +
                "timeout /t 2 /nobreak >nul\r\n" +
                "copy /y \"" + pending + "\" \"" + launcherPath + "\" >nul\r\n" +
                "del \"" + pending + "\" >nul 2>&1\r\n" +
                "del \"%~f0\" >nul 2>&1\r\n";
            File.WriteAllText(bat, script);
            Process.Start(new ProcessStartInfo(bat)
            {
                WorkingDirectory = installRoot,
                WindowStyle = ProcessWindowStyle.Hidden,
                CreateNoWindow = true
            });
        }
        catch
        {
            // Non-fatal: game update already applied.
        }
    }

    void InstallGameFromZip(string zipPath)
    {
        string staging = Path.Combine(installRoot, "Game.staging");
        if (Directory.Exists(staging)) Directory.Delete(staging, true);
        Directory.CreateDirectory(staging);
        ZipFile.ExtractToDirectory(zipPath, staging);

        if (Directory.Exists(gameDir))
        {
            try { Directory.Delete(gameDir, true); }
            catch
            {
                // Fallback rename if files are locked.
                string old = gameDir + ".old-" + DateTime.Now.Ticks;
                Directory.Move(gameDir, old);
                try { Directory.Delete(old, true); } catch { }
            }
        }
        Directory.Move(staging, gameDir);
    }

    void LaunchGameAndExit()
    {
        if (string.IsNullOrEmpty(launchPath) || !File.Exists(launchPath))
        {
            launchPath = FindExe(gameDir);
        }
        if (string.IsNullOrEmpty(launchPath) || !File.Exists(launchPath))
        {
            SetStatus("Game executable not found.", 0);
            busy = false;
            retry.Visible = true;
            return;
        }

        Process.Start(new ProcessStartInfo(launchPath)
        {
            WorkingDirectory = Path.GetDirectoryName(launchPath)
        });
        Close();
    }

    string ReadInstalledVersion()
    {
        try
        {
            if (File.Exists(versionFile)) return File.ReadAllText(versionFile).Trim();
        }
        catch { }
        return "";
    }

    static bool VersionsEqual(string a, string b)
    {
        return string.Equals(NormalizeTag(a), NormalizeTag(b), StringComparison.OrdinalIgnoreCase);
    }

    static string NormalizeTag(string tag)
    {
        if (string.IsNullOrEmpty(tag)) return "";
        tag = tag.Trim();
        if (tag.StartsWith("v", StringComparison.OrdinalIgnoreCase)) tag = tag.Substring(1);
        return tag;
    }

    static bool PathsEqual(string a, string b)
    {
        try
        {
            return string.Equals(Path.GetFullPath(a), Path.GetFullPath(b), StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return string.Equals(a, b, StringComparison.OrdinalIgnoreCase);
        }
    }

    sealed class ReleaseInfo
    {
        public string Tag;
        public string ZipUrl;
        public string SetupUrl;
    }

    ReleaseInfo FetchLatestRelease()
    {
        using (var client = new WebClient())
        {
            client.Headers[HttpRequestHeader.UserAgent] = "ThreeDensityLauncher";
            client.Headers[HttpRequestHeader.Accept] = "application/vnd.github+json";
            string json = client.DownloadString(ApiUrl);

            var info = new ReleaseInfo();
            Match tag = Regex.Match(json, "\"tag_name\"\\s*:\\s*\"([^\"]+)\"");
            if (tag.Success) info.Tag = tag.Groups[1].Value;

            info.ZipUrl = FindAssetUrl(json, "ThreeDensity-Win64.zip") ?? FallbackZip;
            info.SetupUrl = FindAssetUrl(json, "ThreeDensitySetup.exe") ?? FallbackSetup;
            if (string.IsNullOrEmpty(info.Tag)) info.Tag = "latest";
            return info;
        }
    }

    static string FindAssetUrl(string json, string fileName)
    {
        int idx = json.IndexOf(fileName, StringComparison.OrdinalIgnoreCase);
        if (idx < 0) return null;
        int urlKey = json.LastIndexOf("browser_download_url", idx, StringComparison.OrdinalIgnoreCase);
        if (urlKey < 0) return null;
        int http = json.IndexOf("https://", urlKey, StringComparison.OrdinalIgnoreCase);
        if (http < 0) return null;
        int end = json.IndexOf("\"", http);
        if (end < 0) return null;
        return json.Substring(http, end - http);
    }

    static string FindExe(string root)
    {
        if (!Directory.Exists(root)) return null;
        string launcher = Path.Combine(root, "Windows", "threedensity.exe");
        if (File.Exists(launcher)) return launcher;
        string[] matches = Directory.GetFiles(root, "threedensity.exe", SearchOption.AllDirectories);
        if (matches.Length > 0) return matches[0];
        matches = Directory.GetFiles(root, "threedensity-Win64-Shipping.exe", SearchOption.AllDirectories);
        return matches.Length > 0 ? matches[0] : null;
    }

    void Download(string url, string dest)
    {
        using (var client = new WebClient())
        {
            client.Headers[HttpRequestHeader.UserAgent] = "ThreeDensityLauncher";
            client.DownloadProgressChanged += (s, e) =>
            {
                int pct = 15 + (int)(e.ProgressPercentage * 0.65);
                SetStatus(string.Format("Downloading… {0}%", e.ProgressPercentage), pct);
            };
            var done = new ManualResetEvent(false);
            Exception error = null;
            client.DownloadFileCompleted += (s, e) =>
            {
                error = e.Error;
                done.Set();
            };
            client.DownloadFileAsync(new Uri(url), dest);
            done.WaitOne();
            if (error != null) throw error;
        }
    }

    void DownloadQuiet(string url, string dest)
    {
        using (var client = new WebClient())
        {
            client.Headers[HttpRequestHeader.UserAgent] = "ThreeDensityLauncher";
            client.DownloadFile(url, dest);
        }
    }

    static void CreateShortcut(string lnkPath, string target)
    {
        Type t = Type.GetTypeFromProgID("WScript.Shell");
        object shell = Activator.CreateInstance(t);
        object shortcut = t.InvokeMember("CreateShortcut", System.Reflection.BindingFlags.InvokeMethod, null, shell, new object[] { lnkPath });
        Type st = shortcut.GetType();
        st.InvokeMember("TargetPath", System.Reflection.BindingFlags.SetProperty, null, shortcut, new object[] { target });
        st.InvokeMember("WorkingDirectory", System.Reflection.BindingFlags.SetProperty, null, shortcut, new object[] { Path.GetDirectoryName(target) });
        st.InvokeMember("Description", System.Reflection.BindingFlags.SetProperty, null, shortcut, new object[] { "Three Density Launcher" });
        st.InvokeMember("IconLocation", System.Reflection.BindingFlags.SetProperty, null, shortcut, new object[] { target + ",0" });
        st.InvokeMember("Save", System.Reflection.BindingFlags.InvokeMethod, null, shortcut, null);
    }

    void SetStatus(string text, int percent)
    {
        if (IsDisposed) return;
        try
        {
            Invoke(new Action(() =>
            {
                status.Text = text;
                progress.Value = Math.Max(0, Math.Min(100, percent));
            }));
        }
        catch { }
    }
}
