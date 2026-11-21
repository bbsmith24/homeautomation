namespace LightDriverMQTT
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            components = new System.ComponentModel.Container();
            button1 = new Button();
            button2 = new Button();
            button3 = new Button();
            timer1 = new System.Windows.Forms.Timer(components);
            numericUpDown1 = new NumericUpDown();
            label1 = new Label();
            textBox1 = new TextBox();
            ((System.ComponentModel.ISupportInitialize)numericUpDown1).BeginInit();
            SuspendLayout();
            // 
            // button1
            // 
            button1.Location = new Point(12, 96);
            button1.Name = "button1";
            button1.Size = new Size(140, 34);
            button1.TabIndex = 0;
            button1.Text = "Publish";
            button1.UseVisualStyleBackColor = true;
            button1.Click += button1_Click;
            // 
            // button2
            // 
            button2.Location = new Point(12, 12);
            button2.Name = "button2";
            button2.Size = new Size(140, 34);
            button2.TabIndex = 1;
            button2.Text = "Disconnected";
            button2.UseVisualStyleBackColor = true;
            button2.Click += button2_Click;
            // 
            // button3
            // 
            button3.BackColor = Color.Red;
            button3.Location = new Point(173, 12);
            button3.Name = "button3";
            button3.Size = new Size(140, 34);
            button3.TabIndex = 2;
            button3.Text = "Start Timer";
            button3.UseVisualStyleBackColor = false;
            button3.Click += button3_Click;
            // 
            // timer1
            // 
            timer1.Interval = 500;
            timer1.Tick += timer1_Tick;
            // 
            // numericUpDown1
            // 
            numericUpDown1.Increment = new decimal(new int[] { 10, 0, 0, 0 });
            numericUpDown1.Location = new Point(392, 14);
            numericUpDown1.Maximum = new decimal(new int[] { 2500, 0, 0, 0 });
            numericUpDown1.Minimum = new decimal(new int[] { 100, 0, 0, 0 });
            numericUpDown1.Name = "numericUpDown1";
            numericUpDown1.Size = new Size(112, 31);
            numericUpDown1.TabIndex = 3;
            numericUpDown1.Value = new decimal(new int[] { 250, 0, 0, 0 });
            numericUpDown1.ValueChanged += numericUpDown1_ValueChanged;
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new Point(330, 17);
            label1.Name = "label1";
            label1.Size = new Size(56, 25);
            label1.TabIndex = 4;
            label1.Text = "Delay";
            // 
            // textBox1
            // 
            textBox1.Font = new Font("Consolas", 10F);
            textBox1.Location = new Point(158, 100);
            textBox1.Name = "textBox1";
            textBox1.Size = new Size(596, 31);
            textBox1.TabIndex = 5;
            textBox1.Text = "0000000000000000 0000000000000000 0000000000000000";
            // 
            // Form1
            // 
            AcceptButton = button1;
            AutoScaleDimensions = new SizeF(10F, 25F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(800, 450);
            Controls.Add(textBox1);
            Controls.Add(label1);
            Controls.Add(numericUpDown1);
            Controls.Add(button3);
            Controls.Add(button2);
            Controls.Add(button1);
            Name = "Form1";
            Text = "Form1";
            ((System.ComponentModel.ISupportInitialize)numericUpDown1).EndInit();
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private Button button1;
        private Button button2;
        private Button button3;
        private System.Windows.Forms.Timer timer1;
        private NumericUpDown numericUpDown1;
        private Label label1;
        private TextBox textBox1;
    }
}
