using HiveMQtt.Client;
using HiveMQtt.Client.Events;
using HiveMQtt.Client.Results;
using HiveMQtt.MQTT5.ReasonCodes;
using System.Diagnostics.Metrics;
using System.Net;
using System.Security;
using System.Text;

namespace LightDriverMQTT
{
    public partial class Form1 : Form
    {
        UInt16[] messageVal = { 0x0000, 0x0000, 0xFFFF };

        int frameDelay = 100;

        int counter = 0;
        HiveMQClient? client;
        bool isConnected;

        // reconnect after an unexpected disconnect - waits between attempts, last value repeats
        static readonly int[] reconnectDelaysMs = { 1000, 2000, 5000, 10000, 30000 };
        bool reconnecting;       // reconnect loop is running
        bool connecting;         // a connect attempt is in progress
        bool closing;            // form is closing - stop reconnecting

        public Form1()
        {
            InitializeComponent();
            UpdateConnectButton();
        }

        // connect button shows the connection state: green "Connected" or red "Disconnected"
        private void UpdateConnectButton()
        {
            button2.UseVisualStyleBackColor = false;
            button2.BackColor = isConnected ? Color.Green : Color.Red;
            button2.Text = isConnected ? "Connected" : "Disconnected";
        }

        // one connect attempt - true if connected. Runs on the UI thread (no ConfigureAwait(false))
        // so the button can be updated directly
        public async Task<bool> ConnectToBrokerAsync()
        {
            if (connecting)
            {
                return false;
            }
            connecting = true;
            try
            {
                DisposeClient();
                Random random = new Random(DateTime.Now.Millisecond);
                string clientId = "LightDriverMQTT_" + random.Next().ToString();
                // broker and login are in Secrets.cs (not committed - copy Secrets.example.cs)
                string password = Secrets.MqttPassword;
                string username = Secrets.MqttUser;
                string broker = Secrets.MqttBroker;
                var options = new HiveMQClientOptionsBuilder()
                                    .WithBroker(broker)
                                    .WithPort(Secrets.MqttPort)
                                    .WithUserName(username)
                                    .WithPassword(password)
                                    .WithClientId(clientId)
                                    .Build();
                HiveMQClient newClient = new HiveMQClient(options);
                newClient.AfterDisconnect += Client_AfterDisconnect;
                ConnectResult connectResult;
                try
                {
                    connectResult = await newClient.ConnectAsync();
                }
                catch (Exception ex)
                {
                    // broker unreachable, network down, ...
                    System.Diagnostics.Debug.WriteLine("Connect failed! " + ex.Message);
                    newClient.AfterDisconnect -= Client_AfterDisconnect;
                    newClient.Dispose();
                    isConnected = false;
                    return false;
                }
                if (connectResult.ReasonCode == ConnAckReasonCode.Success)
                {
                    client = newClient;
                    isConnected = true;
                    System.Diagnostics.Debug.WriteLine("Connected successfully!");
                    return true;
                }
                System.Diagnostics.Debug.WriteLine("Connect failed! " + connectResult.ToString());
                newClient.AfterDisconnect -= Client_AfterDisconnect;
                newClient.Dispose();
                isConnected = false;
                return false;
            }
            finally
            {
                connecting = false;
                if (!reconnecting)
                {
                    UpdateConnectButton();
                }
            }
        }

        // fired by HiveMQtt on its own thread - hand over to the UI thread
        private void Client_AfterDisconnect(object? sender, AfterDisconnectEventArgs e)
        {
            if (closing || IsDisposed)
            {
                return;
            }
            try
            {
                BeginInvoke(() =>
                {
                    // ignore events from a client that's already been replaced
                    if (!ReferenceEquals(sender, client))
                    {
                        return;
                    }
                    System.Diagnostics.Debug.WriteLine("Disconnected, clean: " + e.CleanDisconnect);
                    ConnectionLost();
                });
            }
            catch (InvalidOperationException)
            {
                // form handle gone (closing)
            }
        }

        // connection dropped (disconnect event or a failed publish) - go red and keep retrying
        private void ConnectionLost()
        {
            isConnected = false;
            UpdateConnectButton();
            _ = ReconnectLoopAsync();
        }

        private async Task ReconnectLoopAsync()
        {
            if (reconnecting || closing)
            {
                return;
            }
            reconnecting = true;
            try
            {
                for (int attempt = 0; !closing; attempt++)
                {
                    int delayMs = reconnectDelaysMs[Math.Min(attempt, reconnectDelaysMs.Length - 1)];
                    await Task.Delay(delayMs);
                    if (closing)
                    {
                        break;
                    }
                    if (await ConnectToBrokerAsync())
                    {
                        break;
                    }
                }
            }
            finally
            {
                reconnecting = false;
                if (!closing)
                {
                    UpdateConnectButton();
                }
            }
        }

        private void DisposeClient()
        {
            if (client == null)
            {
                return;
            }
            client.AfterDisconnect -= Client_AfterDisconnect;
            try
            {
                client.Dispose();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Client dispose: " + ex.Message);
            }
            client = null;
        }

        // Publish: stop the timer pattern and send the pattern box - 3 x 16 '0'/'1' separated by spaces
        private async void button1_Click(object sender, EventArgs e)
        {
            if (!isConnected || client == null)
            {
                MessageBox.Show("Connect to the MQTT broker before publishing.", "Not Connected", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // same format the timer sends: three 16 character binary fields, any whitespace between
            string[] fields = textBox1.Text.Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries);
            if (fields.Length != 3 || fields.Any(f => f.Length != 16 || f.Any(c => c != '0' && c != '1')))
            {
                MessageBox.Show("Enter three 16 digit binary values (0 and 1 only) separated by spaces, e.g.\n\n1010101010101010 0000111100001111 1111000011110000",
                                "Invalid Pattern", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                textBox1.Focus();
                return;
            }
            string payloadStr = string.Join(" ", fields);
            textBox1.Text = payloadStr;   // show it tidied up

            // the timer would overwrite the pattern on its next tick
            timer1.Enabled = false;
            button3.Text = "Start Timer";
            button3.BackColor = Color.Red;

            try
            {
                await client.PublishAsync("MQTTXMAS/Command", payloadStr);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Publish failed: " + ex.Message);
                ConnectionLost();
            }
        }

        private async void button2_Click(object sender, EventArgs e)
        {
            // already connected, or the reconnect loop is already on it
            if (isConnected || reconnecting || connecting)
            {
                return;
            }
            if (!await ConnectToBrokerAsync())
            {
                // first connect failed - nothing to resume, so stop the timer
                timer1.Enabled = false;
                button3.Text = "Start Timer";
                button3.BackColor = Color.Red;
            }
        }

        private async void timer1_Tick(object sender, EventArgs e)
        {
            String payloadStr;
            if (!isConnected || client == null)
            {
                if (reconnecting)
                {
                    return;   // keep the timer running, the pattern resumes once reconnected
                }
                timer1.Enabled = false;
                counter = 0;
                button3.Text = "Start Timer";
                button3.BackColor = Color.Red;
                return;
            }
            if (counter < 0)
            {
                messageVal[0] = 0x0000;
                messageVal[1] = 0x0000;
                messageVal[2] = 0x0000;
            }
            else
            {
                messageVal[0] = (UInt16)(messageVal[0] | (1 << counter));
                messageVal[1] = (UInt16)((0x0000 | (1 << counter)));
                messageVal[2] = (UInt16)((0x0000 | (1 << (15 - counter))));
            }
            payloadStr = "";
            for (int idx = 0; idx < 3; idx++)
            {
                payloadStr += messageVal[idx].ToString("B16") + " ";
            }

            try
            {
                await client.PublishAsync("MQTTXMAS/Command", payloadStr);
            }
            catch (Exception ex)
            {
                // connection dropped between ticks - retry the same frame after reconnecting
                System.Diagnostics.Debug.WriteLine("Publish failed: " + ex.Message);
                ConnectionLost();
                return;
            }
            //System.Diagnostics.Debug.WriteLine(counter.ToString() + " " +payloadStr);

            counter = counter + 1 < 16 ? counter + 1 : -1;
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if (timer1.Enabled)
            {
                timer1.Enabled = false;
                button3.Text = "Start Timer";
                button3.BackColor = Color.Red;
            }
            else
            {
                if (!isConnected || client == null)
                {
                    MessageBox.Show("Connect to the MQTT broker before starting the timer.", "Not Connected", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }

                timer1.Enabled = true;
                button3.Text = "Stop Timer";
                button3.BackColor = Color.Green;
            }
        }

        // disconnect cleanly on exit so the broker doesn't wait for a keep-alive timeout
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            closing = true;
            timer1.Enabled = false;
            HiveMQClient? c = client;
            if (c != null)
            {
                c.AfterDisconnect -= Client_AfterDisconnect;
                if (isConnected)
                {
                    try
                    {
                        // off the UI thread so it can't deadlock waiting for this thread
                        Task.Run(() => c.DisconnectAsync()).Wait(2000);
                    }
                    catch (Exception ex)
                    {
                        System.Diagnostics.Debug.WriteLine("Disconnect on close: " + ex.Message);
                    }
                }
                DisposeClient();
            }
            base.OnFormClosing(e);
        }

        public static string ReverseString(string input)
        {
            if (string.IsNullOrEmpty(input)) return input;

            char[] chars = input.ToCharArray();
            Array.Reverse(chars);
            return new string(chars);
        }

        private void numericUpDown1_ValueChanged(object sender, EventArgs e)
        {
            frameDelay = (int)numericUpDown1.Value;
            timer1.Interval = frameDelay;
        }
    }
}
