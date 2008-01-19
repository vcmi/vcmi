#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace Wpasuj {

	/// <summary>
	/// Summary for obrazek
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class obrazek : public System::Windows::Forms::Form
	{
	public:
		obrazek(System::Drawing::Image^ img)
		{
			InitializeComponent(img);
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~obrazek()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::PictureBox^  pictureBox1;
	private: System::Windows::Forms::TrackBar^  trackBar1;
	private: System::Windows::Forms::Label^  Transparency;
	protected: 

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(System::Drawing::Image^ img)
		{
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox1))->BeginInit();
			this->SuspendLayout();
			// 
			// pictureBox1
			// 
			this->pictureBox1->Location = System::Drawing::Point(12, 12);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(img->Width,img->Height);//(268, 242);
			this->pictureBox1->TabIndex = 0;
			this->pictureBox1->TabStop = false;
			this->pictureBox1->Image = img;
			this->pictureBox1->Click += gcnew System::EventHandler(this,&obrazek::obrazek_Click);


			
			this->Transparency = (gcnew System::Windows::Forms::Label());
			this->Transparency->AutoSize = true;
			this->Transparency->Location = System::Drawing::Point(12, img->Height+70);
			this->Transparency->Name = L"label13";
			this->Transparency->Size = System::Drawing::Size(110, 13);
			this->Transparency->TabIndex = 21;
			this->Transparency->Text = L"Window transparency";


			this->trackBar1 = (gcnew System::Windows::Forms::TrackBar());
			this->trackBar1->Location = System::Drawing::Point(12, img->Height+24);
			this->trackBar1->Name = L"trackBar1";
			this->trackBar1->Size = System::Drawing::Size(img->Width, 45);
			this->trackBar1->TabIndex = 26;
			this->trackBar1->Minimum = 0;
			this->trackBar1->Maximum = 255;
			this->trackBar1->Value = 255;
			this->trackBar1->TickFrequency=1;
			this->trackBar1->ValueChanged += gcnew System::EventHandler(this,&obrazek::suwakiemRuszono);

			// 
			// obrazek
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(img->Width+24,img->Height+94);//(292, 266);
			this->Controls->Add(this->pictureBox1);
			this->Controls->Add(this->trackBar1);
			this->Controls->Add(this->Transparency);
			this->Click += gcnew System::EventHandler(this,&obrazek::obrazek_Click);
			this->Name = L"obrazek";
			this->Text = L"Picture window";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox1))->EndInit();
			this->ResumeLayout(false);


		}
#pragma endregion

	private: System::Void obrazek_Click(System::Object^  sender, System::EventArgs^  e) {
				Close();
			 }
	private: System::Void suwakiemRuszono(System::Object^  sender, System::EventArgs^  e) {
				 Opacity = trackBar1->Value/((double)255);
			 }
	};
}
