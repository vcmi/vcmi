using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Collections;
namespace DefSorter
{
    public partial class Form1 : Form
    {
        int bledy;
        public Form1()
        {
            InitializeComponent();
            int ile = 0;
            bledy = 0;
            string[] pliczki = Directory.GetFiles(Directory.GetCurrentDirectory());
            foreach (string ss in pliczki)
            {
                if (!(ss.EndsWith(".DEF")))
                    continue;
                else ile++;
            }
            label2.Text = ile.ToString();
            progressBar1.Visible = false;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            int s12 = 0, s13 = 0, s14 = 0, s15 = 0, s16 = 0, s17 = 0, s18 = 0,
                s19 = 0, s20 = 0, s21 = 0;
            Form2 pozegnanie = new Form2();
            ArrayList defy = new ArrayList();
            string[] pliczki = Directory.GetFiles(Directory.GetCurrentDirectory(),"*.DEF");
            progressBar1.Visible = true;
            progressBar1.Minimum = progressBar1.Value = 0;
            progressBar1.Maximum = Convert.ToInt32(label2.Text);
            foreach (string ss in pliczki)
            {
                if (!(ss.EndsWith(".DEF")))
                    continue;
                FileStream czytacz = File.OpenRead(ss);
                int coTo = czytacz.ReadByte();
                czytacz.Close();
                int poczP = ss.LastIndexOf('\\');
                string nazwa = ss.Substring(poczP + 1, ss.Length - poczP - 1);
                try
                {
                    switch (coTo)
                    {
                        case 64:
                            if (!Directory.Exists("40Spell"))
                                Directory.CreateDirectory("40Spell");
                            File.Copy(ss, "40Spell\\" + nazwa);
                            s12++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSK", "40Spell\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSG", "40Spell\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSG");
                            }
                            break;
                        case 65:
                            if (!Directory.Exists("41SpriteDef"))
                                Directory.CreateDirectory("41SpriteDef");
                            File.Copy(ss, "41SpriteDef\\" + nazwa);
                            s13++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSK", "41SpriteDef\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSG", "41SpriteDef\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSG");
                            }
                            break;
                        case 66:
                            if (!Directory.Exists("42Creature"))
                                Directory.CreateDirectory("42Creature");
                            File.Copy(ss, "42Creature\\" + nazwa);
                            s14++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSK", "42Creature\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSG", "42Creature\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSG");
                            }
                            break;
                        case 67:
                            if (!Directory.Exists("43AdvObject"))
                                Directory.CreateDirectory("43AdvObject");
                            File.Copy(ss, "43AdvObject\\" + nazwa);
                            s15++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSK", "43AdvObject\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSG", "43AdvObject\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSG");
                            }
                            break;
                        case 68:
                            if (!Directory.Exists("44Hero"))
                                Directory.CreateDirectory("44Hero");
                            File.Copy(ss, "44Hero\\" + nazwa);
                            s16++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSK", "44Hero\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSG", "44Hero\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSG");
                            }
                            break;
                        case 69:
                            if (!Directory.Exists("45Terrain"))
                                Directory.CreateDirectory("45Terrain");
                            File.Copy(ss, "45Terrain\\" + nazwa);
                            s17++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4)+".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4)+".MSK","45Terrain\\"+nazwa.Substring(0, nazwa.Length - 4)+".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4)+".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4)+".MSG","45Terrain\\"+nazwa.Substring(0, nazwa.Length - 4)+".MSG");
                            }
                            break;
                        case 70:
                            if (!Directory.Exists("46Cursor"))
                                Directory.CreateDirectory("46Cursor");
                            File.Copy(ss, "46Cursor\\" + nazwa);
                            s18++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSK", "46Cursor\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSG", "46Cursor\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSG");
                            }
                            break;
                        case 71:
                            if (!Directory.Exists("47Interface"))
                                Directory.CreateDirectory("47Interface");
                            File.Copy(ss, "47Interface\\" + nazwa);
                            s19++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSK", "47Interface\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSG", "47Interface\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSG");
                            }
                            break;
                        case 72:
                            if (!Directory.Exists("48Spriteframe"))
                                Directory.CreateDirectory("48Spriteframe");
                            File.Copy(ss, "48Spriteframe\\" + nazwa);
                            s20++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSK", "48Spriteframe\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSG", "48Spriteframe\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSG");
                            }
                            break;
                        case 73:
                            if (!Directory.Exists("49CombatHero"))
                                Directory.CreateDirectory("49CombatHero");
                            File.Copy(ss, "49CombatHero\\" + nazwa);
                            s21++;
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSK"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSK", "49CombatHero\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSK");
                            }
                            if (File.Exists(nazwa.Substring(0, nazwa.Length - 4) + ".MSG"))
                            {
                                File.Copy(nazwa.Substring(0, nazwa.Length - 4) + ".MSG", "49CombatHero\\" + nazwa.Substring(0, nazwa.Length - 4) + ".MSG");
                            }
                            break;
                    }
                }
                catch (Exception ee)
                {
                    bledy++;
                    Form3 niedobrze = new Form3();
                    niedobrze.textBox1.Text += ee.Message;
                    DialogResult res = niedobrze.ShowDialog();
                    switch (res)
                    {
                        case DialogResult.Ignore:
                            break;
                        case DialogResult.Cancel:
                            try { Directory.Delete("40Spell"); }catch(Exception){};
                            try { Directory.Delete("41SpriteDef"); }catch(Exception){};
                            try { Directory.Delete("42Creature"); }catch(Exception){};
                            try { Directory.Delete("43AdvObject"); }catch(Exception){};
                            try { Directory.Delete("45Terrain"); }catch(Exception){};
                            try { Directory.Delete("46Cursor"); }catch(Exception){};
                            try { Directory.Delete("47Interface"); }catch(Exception){};
                            try { Directory.Delete("48Spriteframe"); }catch(Exception){};
                            try { Directory.Delete("49CombatHero"); }catch(Exception){};
                            Application.Exit();
                            break;
                    }
                }
                progressBar1.Value++;
                if (progressBar1.Value % 100 == 5)
                    this.Update();
            }
            progressBar1.Visible = false;
            pozegnanie.label12.Text = s12.ToString();
            pozegnanie.label13.Text = s13.ToString();
            pozegnanie.label14.Text = s14.ToString();
            pozegnanie.label15.Text = s15.ToString();
            pozegnanie.label16.Text = s16.ToString();
            pozegnanie.label17.Text = s17.ToString();
            pozegnanie.label18.Text = s18.ToString();
            pozegnanie.label19.Text = s19.ToString();
            pozegnanie.label20.Text = s20.ToString();
            pozegnanie.label21.Text = s21.ToString();
            pozegnanie.label34.Text = bledy.ToString();
            if (bledy == 0)
                pozegnanie.label35.Text = ":)";
            pozegnanie.Show();
            pozegnanie.Activate();
        }
    }
}