#pragma once
#include <string>
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace wyprujdef {

	/// <summary>
	/// Summary for Oknopruj
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Oknopruj : public System::Windows::Forms::Form
	{
	public:

		static std::string ToString(System::String^ src);
		void runSearch();
		void wyprujDefyZPlikow(array<String^> ^pliki);

		Oknopruj(void)
		{
			InitializeComponent();
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Oknopruj()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::MenuStrip^  menuStrip1;
	protected: 
	private: System::Windows::Forms::ToolStripMenuItem^  rUNToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  getProgramDirToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  browseToolStripMenuItem;

	private: System::Windows::Forms::ToolStripMenuItem^  vCMIHomepageToolStripMenuItem;
	private: System::Windows::Forms::ProgressBar^  progressBar1;
	private: System::Windows::Forms::TextBox^  pathBox;

	private: System::Windows::Forms::Label^  label1;
	private: System::DirectoryServices::DirectorySearcher^  directorySearcher1;
	private: System::Windows::Forms::FolderBrowserDialog^  folderBrowserDialog1;
	private: System::Windows::Forms::RadioButton^  rfirst;
	private: System::Windows::Forms::RadioButton^  rall;





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
		void InitializeComponent(void)
		{
			this->menuStrip1 = (gcnew System::Windows::Forms::MenuStrip());
			this->rUNToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->getProgramDirToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->browseToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->vCMIHomepageToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->progressBar1 = (gcnew System::Windows::Forms::ProgressBar());
			this->pathBox = (gcnew System::Windows::Forms::TextBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->directorySearcher1 = (gcnew System::DirectoryServices::DirectorySearcher());
			this->folderBrowserDialog1 = (gcnew System::Windows::Forms::FolderBrowserDialog());
			this->rfirst = (gcnew System::Windows::Forms::RadioButton());
			this->rall = (gcnew System::Windows::Forms::RadioButton());
			this->menuStrip1->SuspendLayout();
			this->SuspendLayout();
			// 
			// menuStrip1
			// 
			this->menuStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(4) {this->rUNToolStripMenuItem, 
				this->getProgramDirToolStripMenuItem, this->browseToolStripMenuItem, this->vCMIHomepageToolStripMenuItem});
			this->menuStrip1->Location = System::Drawing::Point(0, 0);
			this->menuStrip1->Name = L"menuStrip1";
			this->menuStrip1->Size = System::Drawing::Size(308, 24);
			this->menuStrip1->TabIndex = 0;
			this->menuStrip1->Text = L"menuStrip1";
			// 
			// rUNToolStripMenuItem
			// 
			this->rUNToolStripMenuItem->Name = L"rUNToolStripMenuItem";
			this->rUNToolStripMenuItem->Size = System::Drawing::Size(44, 20);
			this->rUNToolStripMenuItem->Text = L"RUN!";
			this->rUNToolStripMenuItem->Click += gcnew System::EventHandler(this, &Oknopruj::rUNToolStripMenuItem_Click);
			// 
			// getProgramDirToolStripMenuItem
			// 
			this->getProgramDirToolStripMenuItem->Name = L"getProgramDirToolStripMenuItem";
			this->getProgramDirToolStripMenuItem->Size = System::Drawing::Size(94, 20);
			this->getProgramDirToolStripMenuItem->Text = L"Get program dir";
			this->getProgramDirToolStripMenuItem->Click += gcnew System::EventHandler(this, &Oknopruj::getProgramDirToolStripMenuItem_Click);
			// 
			// browseToolStripMenuItem
			// 
			this->browseToolStripMenuItem->Name = L"browseToolStripMenuItem";
			this->browseToolStripMenuItem->Size = System::Drawing::Size(66, 20);
			this->browseToolStripMenuItem->Text = L"Browse...";
			this->browseToolStripMenuItem->Click += gcnew System::EventHandler(this, &Oknopruj::browseToolStripMenuItem_Click);
			// 
			// vCMIHomepageToolStripMenuItem
			// 
			this->vCMIHomepageToolStripMenuItem->Name = L"vCMIHomepageToolStripMenuItem";
			this->vCMIHomepageToolStripMenuItem->Size = System::Drawing::Size(97, 20);
			this->vCMIHomepageToolStripMenuItem->Text = L"VCMI homepage";
			this->vCMIHomepageToolStripMenuItem->Click += gcnew System::EventHandler(this, &Oknopruj::vCMIHomepageToolStripMenuItem_Click);
			// 
			// progressBar1
			// 
			this->progressBar1->Location = System::Drawing::Point(12, 96);
			this->progressBar1->Name = L"progressBar1";
			this->progressBar1->Size = System::Drawing::Size(284, 14);
			this->progressBar1->TabIndex = 1;
			// 
			// pathBox
			// 
			this->pathBox->Location = System::Drawing::Point(12, 50);
			this->pathBox->Name = L"pathBox";
			this->pathBox->Size = System::Drawing::Size(276, 20);
			this->pathBox->TabIndex = 2;
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(137, 34);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(32, 13);
			this->label1->TabIndex = 3;
			this->label1->Text = L"Path:";
			// 
			// directorySearcher1
			// 
			this->directorySearcher1->ClientTimeout = System::TimeSpan::Parse(L"-00:00:01");
			this->directorySearcher1->ServerPageTimeLimit = System::TimeSpan::Parse(L"-00:00:01");
			this->directorySearcher1->ServerTimeLimit = System::TimeSpan::Parse(L"-00:00:01");
			// 
			// rfirst
			// 
			this->rfirst->AutoSize = true;
			this->rfirst->Checked = true;
			this->rfirst->Location = System::Drawing::Point(12, 73);
			this->rfirst->Name = L"rfirst";
			this->rfirst->Size = System::Drawing::Size(128, 17);
			this->rfirst->TabIndex = 4;
			this->rfirst->TabStop = true;
			this->rfirst->Text = L"Extract only first frame";
			this->rfirst->UseVisualStyleBackColor = true;
			// 
			// rall
			// 
			this->rall->AutoSize = true;
			this->rall->Location = System::Drawing::Point(183, 73);
			this->rall->Name = L"rall";
			this->rall->Size = System::Drawing::Size(105, 17);
			this->rall->TabIndex = 4;
			this->rall->Text = L"Extract all frames";
			this->rall->UseVisualStyleBackColor = true;
			// 
			// Oknopruj
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(308, 120);
			this->Controls->Add(this->rall);
			this->Controls->Add(this->rfirst);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->pathBox);
			this->Controls->Add(this->progressBar1);
			this->Controls->Add(this->menuStrip1);
			this->MainMenuStrip = this->menuStrip1;
			this->MaximizeBox = false;
			this->Name = L"Oknopruj";
			this->ShowIcon = false;
			this->Text = L"Defopruj 1.0";
			this->Load += gcnew System::EventHandler(this, &Oknopruj::Oknopruj_Load);
			this->menuStrip1->ResumeLayout(false);
			this->menuStrip1->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
private: System::Void rUNToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) 
		 {
			 runSearch();
		 }
private: System::Void Oknopruj_Load(System::Object^  sender, System::EventArgs^  e) 
		 {
			 pathBox->Text = IO::Directory::GetCurrentDirectory();
		 }
private: System::Void getProgramDirToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) 
		 {
			 pathBox->Text = IO::Directory::GetCurrentDirectory();
		 }
private: System::Void browseToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) 
		 {
			 if (folderBrowserDialog1->ShowDialog()==System::Windows::Forms::DialogResult::OK)
			 {
				 pathBox->Text = folderBrowserDialog1->SelectedPath;
			 }
		 }
private: System::Void aboutToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) 
		 {

		 }
private: System::Void vCMIHomepageToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) 
		 {
			 Diagnostics::Process::Start(gcnew String("http://antypika.aplus.pl/vcmi"));
		 }
};
}
