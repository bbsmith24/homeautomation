// template for Secrets.cs - copy this file to Secrets.cs and fill in your broker details.
// Secrets.cs is gitignored; this example is excluded from the build in LightDriverMQTT.csproj
namespace LightDriverMQTT
{
    internal static class Secrets
    {
        public const string MqttBroker = "your-mqtt-broker-ip";
        public const int MqttPort = 1883;
        public const string MqttUser = "your-mqtt-user";
        public const string MqttPassword = "your-mqtt-password";
    }
}
