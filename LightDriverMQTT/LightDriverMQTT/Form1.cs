using HiveMQtt.Client;
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

        public Form1()
        {
            InitializeComponent();
        }

        public async Task ConnectToBrokerAsync()
        {
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
            var connectResult = await newClient.ConnectAsync().ConfigureAwait(false);
            if (connectResult.ReasonCode == ConnAckReasonCode.Success)
            {
                client = newClient;
                isConnected = true;
                System.Diagnostics.Debug.WriteLine("Connected successfully!");
            }
            else
            {
                client = null;
                isConnected = false;
                timer1.Enabled = false;
                button3.Text = "Start Timer";
                button3.BackColor = Color.Red;
                System.Diagnostics.Debug.WriteLine("Connected failed! " + connectResult.ToString());
            }
        }

        private async void button1_Click(object sender, EventArgs e)
        {
            if (!isConnected || client == null)
            {
                MessageBox.Show("Connect to the MQTT broker before publishing.", "Not Connected", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            await client.PublishAsync("topic1/example", "Hello Payload").ConfigureAwait(false);
            //System.Diagnostics.Debug.WriteLine("button1_Click publish result " + publishResult.Message);

        }

        private async void button2_Click(object sender, EventArgs e)
        {
            await ConnectToBrokerAsync();

        }

        private async void timer1_Tick(object sender, EventArgs e)
        {
            String payloadStr;
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
            if (!isConnected || client == null)
            {
                timer1.Enabled = false;
                counter = 0;
                button3.Text = "Start Timer";
                button3.BackColor = Color.Red;
                return;
            }

            await client.PublishAsync("MQTTXMAS/Command", payloadStr).ConfigureAwait(false);
            //System.Diagnostics.Debug.WriteLine(counter.ToString() + " " +payloadStr);
            //System.Diagnostics.Debug.WriteLine("timer1_Tick publish result " + publishResult.Message);

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
