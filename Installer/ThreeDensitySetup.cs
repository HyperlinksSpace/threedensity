using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Threading;
using System.Windows.Forms;

internal static class Program
{
    [STAThread]
    static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new SetupForm());
    }
}

public sealed class SetupForm : Form
{
    const string ApiUrl = "https://api.github.com/repos/HyperlinksSpace/threedensity/releases/latest";
    const string FallbackZip = "https://github.com/HyperlinksSpace/threedensity/releases/latest/download/ThreeDensity-Win64.zip";

    readonly PictureBox logo;
    readonly Label subtitle;
    readonly Label status;
    readonly ProgressBar progress;
    readonly Button action;
    readonly CheckBox launchWhenDone;
    string installDir;
    string launchPath;

    public SetupForm()
    {
        Text = "Three Density Setup";
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(540, 390);
        BackColor = Color.FromArgb(12, 12, 11);
        ForeColor = Color.FromArgb(230, 224, 214);
        Font = new Font("Segoe UI", 10f);

        try
        {
            Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath);
        }
        catch
        {
        }

        installDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ThreeDensity");

        logo = new PictureBox
        {
            Bounds = new Rectangle(24, 18, 72, 72),
            SizeMode = PictureBoxSizeMode.Zoom,
            BackColor = Color.Transparent
        };
        try
        {
            logo.Image = LogoData.LoadMark();
        }
        catch
        {
        }

        Label title = new Label
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
            Text = "Installer downloads the latest Windows build\nand creates a desktop shortcut."
        };
        status = new Label
        {
            AutoSize = false,
            Bounds = new Rectangle(28, 180, 480, 40),
            Text = "Ready to install."
        };
        progress = new ProgressBar
        {
            Bounds = new Rectangle(28, 230, 484, 18),
            Style = ProgressBarStyle.Continuous
        };
        launchWhenDone = new CheckBox
        {
            Bounds = new Rectangle(28, 268, 300, 24),
            Checked = true,
            ForeColor = Color.FromArgb(200, 196, 188),
            Text = "Launch game when finished"
        };
        action = new Button
        {
            Bounds = new Rectangle(28, 310, 170, 40),
            FlatStyle = FlatStyle.Flat,
            BackColor = Color.FromArgb(196, 92, 28),
            ForeColor = Color.White,
            Font = new Font("Segoe UI", 11f, FontStyle.Bold),
            Text = "Install"
        };
        action.FlatAppearance.BorderSize = 0;
        action.Click += StartInstallClick;

        Controls.Add(logo);
        Controls.Add(title);
        Controls.Add(subtitle);
        Controls.Add(status);
        Controls.Add(progress);
        Controls.Add(launchWhenDone);
        Controls.Add(action);
    }

    void StartInstallClick(object sender, EventArgs e)
    {
        StartInstall();
    }

    void StartInstall()
    {
        action.Enabled = false;
        ThreadPool.QueueUserWorkItem(state =>
        {
            try
            {
                ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
                Directory.CreateDirectory(installDir);
                string zipPath = Path.Combine(installDir, "ThreeDensity-Win64.zip");

                SetStatus("Finding latest release…", 5);
                string zipUrl = ResolveZipUrl() ?? FallbackZip;

                SetStatus("Downloading game files…", 10);
                Download(zipUrl, zipPath);

                SetStatus("Installing…", 82);
                string extractDir = Path.Combine(installDir, "Game");
                if (Directory.Exists(extractDir))
                {
                    Directory.Delete(extractDir, true);
                }
                Directory.CreateDirectory(extractDir);
                ZipFile.ExtractToDirectory(zipPath, extractDir);
                try { File.Delete(zipPath); } catch { }

                launchPath = FindExe(extractDir);
                if (string.IsNullOrEmpty(launchPath))
                {
                    throw new InvalidOperationException("Could not find threedensity.exe after install.");
                }

                SetStatus("Creating shortcut…", 94);
                CreateShortcut(launchPath);

                SetStatus("Installed. Ready to play.", 100);
                Invoke(new Action(() =>
                {
                    action.Text = "Play";
                    action.Enabled = true;
                    action.Click -= StartInstallClick;
                    action.Click += PlayClick;
                    if (launchWhenDone.Checked)
                    {
                        LaunchGame();
                    }
                }));
            }
            catch (Exception ex)
            {
                SetStatus("Install failed: " + ex.Message, 0);
                Invoke(new Action(() => { action.Enabled = true; action.Text = "Retry"; }));
            }
        });
    }

    void PlayClick(object sender, EventArgs e)
    {
        LaunchGame();
    }

    void LaunchGame()
    {
        if (string.IsNullOrEmpty(launchPath) || !File.Exists(launchPath))
        {
            launchPath = FindExe(Path.Combine(installDir, "Game"));
        }
        if (!string.IsNullOrEmpty(launchPath) && File.Exists(launchPath))
        {
            Process.Start(new ProcessStartInfo(launchPath) { WorkingDirectory = Path.GetDirectoryName(launchPath) });
            Close();
        }
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

    string ResolveZipUrl()
    {
        try
        {
            using (var client = new WebClient())
            {
                client.Headers[HttpRequestHeader.UserAgent] = "ThreeDensitySetup";
                client.Headers[HttpRequestHeader.Accept] = "application/vnd.github+json";
                string json = client.DownloadString(ApiUrl);
                int idx = json.IndexOf("ThreeDensity-Win64.zip", StringComparison.OrdinalIgnoreCase);
                if (idx < 0) return null;
                int urlKey = json.LastIndexOf("browser_download_url", idx, StringComparison.OrdinalIgnoreCase);
                if (urlKey < 0) return null;
                int http = json.IndexOf("https://", urlKey, StringComparison.OrdinalIgnoreCase);
                int end = json.IndexOf("\"", http);
                return json.Substring(http, end - http);
            }
        }
        catch
        {
            return null;
        }
    }

    void Download(string url, string dest)
    {
        using (var client = new WebClient())
        {
            client.Headers[HttpRequestHeader.UserAgent] = "ThreeDensitySetup";
            client.DownloadProgressChanged += (s, e) =>
            {
                int pct = 10 + (int)(e.ProgressPercentage * 0.7);
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

    void CreateShortcut(string target)
    {
        string desktop = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
        string startMenu = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.StartMenu), "Programs");
        Directory.CreateDirectory(startMenu);
        WriteShortcut(Path.Combine(desktop, "Three Density.lnk"), target);
        WriteShortcut(Path.Combine(startMenu, "Three Density.lnk"), target);
    }

    static void WriteShortcut(string lnkPath, string target)
    {
        Type t = Type.GetTypeFromProgID("WScript.Shell");
        object shell = Activator.CreateInstance(t);
        object shortcut = t.InvokeMember("CreateShortcut", System.Reflection.BindingFlags.InvokeMethod, null, shell, new object[] { lnkPath });
        Type st = shortcut.GetType();
        st.InvokeMember("TargetPath", System.Reflection.BindingFlags.SetProperty, null, shortcut, new object[] { target });
        st.InvokeMember("WorkingDirectory", System.Reflection.BindingFlags.SetProperty, null, shortcut, new object[] { Path.GetDirectoryName(target) });
        st.InvokeMember("Description", System.Reflection.BindingFlags.SetProperty, null, shortcut, new object[] { "Three Density" });
        string ico = Path.Combine(Path.GetDirectoryName(Application.ExecutablePath) ?? "", "ThreeDensity.ico");
        if (File.Exists(ico))
        {
            st.InvokeMember("IconLocation", System.Reflection.BindingFlags.SetProperty, null, shortcut, new object[] { ico });
        }
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
