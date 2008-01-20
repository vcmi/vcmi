#pragma once
#include <string>
#include <fstream>

namespace Obwodziciel {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

std::string ToString(System::String^ src)
{
	std::string dest;
	using namespace System::Runtime::InteropServices;
	const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(src)).ToPointer();
	dest = chars;
	Marshal::FreeHGlobal(IntPtr((void*)chars));
	return dest;
}
	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		value struct BuildingEntry
		{
			int townID, ID;
			String^ defname;
		};
		Form1(void)
		{
			InitializeComponent();
			source1->Text = IO::Directory::GetCurrentDirectory();
			source2->Text = IO::Directory::GetCurrentDirectory();

			IO::StreamReader ^sr = gcnew IO::StreamReader(IO::Directory::GetCurrentDirectory()+L"\\BNAMES.TXT");
			int i=0;
			while(!sr->EndOfStream)
			{
				String ^ n = sr->ReadLine();
				//n = (i.ToString()) + L". " + n;
				buildingTypes->Items->Add(n);
				i++;
			}
			buildingTypes->SelectedIndex=0;
			sr->Close();
			delete sr;

			sr = gcnew IO::StreamReader(IO::Directory::GetCurrentDirectory()+L"\\TOWNTYPE.TXT");
			while(!sr->EndOfStream)
			{
				String ^ n = sr->ReadLine();
				castleType->Items->Add(n);
			}
			castleType->SelectedIndex = 0;

			std::ifstream bb("buildings.txt");
			std::string pom;
			while(!bb.eof())
			{
				BuildingEntry be;
				bb >> be.townID >> be.ID >> pom >> i >> i;
				be.defname = gcnew String(pom.c_str());
				lista.Add(be);
			}
			bb.close();
			bb.clear();

			out = gcnew IO::StreamWriter("wynik.txt");
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
			out->Close();
		}
	private: System::Windows::Forms::PictureBox^  pictureBox1;
	protected: 
	private: System::Windows::Forms::PictureBox^  pictureBox2;
	private: System::Windows::Forms::ListBox^  listBox1;
	private: System::Windows::Forms::ListBox^  listBox2;
	private: System::Windows::Forms::TextBox^  searchpat1;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::TextBox^  searchpat2;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Button^  search1;
	private: System::Windows::Forms::Button^  search2;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::Button^  prev1;
	private: System::Windows::Forms::Button^  next1;
	private: System::Windows::Forms::Button^  prev2;
	private: System::Windows::Forms::Button^  next2;
	private: System::Windows::Forms::Button^  assign1;
	private: System::Windows::Forms::Button^  assign2;
	private: System::Windows::Forms::ListBox^  possibilites1;
	private: System::Windows::Forms::ListBox^  possibilites2;
	private: System::Windows::Forms::Button^  confirm1;
	private: System::Windows::Forms::Button^  confirm2;
	private: System::Windows::Forms::TextBox^  source1;
	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::TextBox^  source2;
	private: System::Windows::Forms::Label^  label6;
	private: System::Windows::Forms::Button^  browse1;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::Button^  browse2;
	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::Label^  label8;
	private: System::Windows::Forms::Label^  label9;



	private: System::Windows::Forms::Label^  label10;
	private: System::Windows::Forms::FolderBrowserDialog^  folderBrowserDialog1;


	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>

		int b1, b2;
		IO::StreamWriter ^out;
		System::Collections::Generic::List<BuildingEntry> lista;
	private: System::Windows::Forms::ComboBox^  buildingTypes;
private: System::Windows::Forms::ComboBox^  castleType;

		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->pictureBox2 = (gcnew System::Windows::Forms::PictureBox());
			this->listBox1 = (gcnew System::Windows::Forms::ListBox());
			this->listBox2 = (gcnew System::Windows::Forms::ListBox());
			this->searchpat1 = (gcnew System::Windows::Forms::TextBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->searchpat2 = (gcnew System::Windows::Forms::TextBox());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->search1 = (gcnew System::Windows::Forms::Button());
			this->search2 = (gcnew System::Windows::Forms::Button());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->prev1 = (gcnew System::Windows::Forms::Button());
			this->next1 = (gcnew System::Windows::Forms::Button());
			this->prev2 = (gcnew System::Windows::Forms::Button());
			this->next2 = (gcnew System::Windows::Forms::Button());
			this->assign1 = (gcnew System::Windows::Forms::Button());
			this->assign2 = (gcnew System::Windows::Forms::Button());
			this->possibilites1 = (gcnew System::Windows::Forms::ListBox());
			this->possibilites2 = (gcnew System::Windows::Forms::ListBox());
			this->confirm1 = (gcnew System::Windows::Forms::Button());
			this->confirm2 = (gcnew System::Windows::Forms::Button());
			this->source1 = (gcnew System::Windows::Forms::TextBox());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->source2 = (gcnew System::Windows::Forms::TextBox());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->browse1 = (gcnew System::Windows::Forms::Button());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->browse2 = (gcnew System::Windows::Forms::Button());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->label8 = (gcnew System::Windows::Forms::Label());
			this->label9 = (gcnew System::Windows::Forms::Label());
			this->label10 = (gcnew System::Windows::Forms::Label());
			this->folderBrowserDialog1 = (gcnew System::Windows::Forms::FolderBrowserDialog());
			this->buildingTypes = (gcnew System::Windows::Forms::ComboBox());
			this->castleType = (gcnew System::Windows::Forms::ComboBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox1))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox2))->BeginInit();
			this->SuspendLayout();
			// 
			// pictureBox1
			// 
			this->pictureBox1->Location = System::Drawing::Point(12, 38);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(450, 298);
			this->pictureBox1->TabIndex = 0;
			this->pictureBox1->TabStop = false;
			// 
			// pictureBox2
			// 
			this->pictureBox2->Location = System::Drawing::Point(522, 38);
			this->pictureBox2->Name = L"pictureBox2";
			this->pictureBox2->Size = System::Drawing::Size(450, 298);
			this->pictureBox2->TabIndex = 0;
			this->pictureBox2->TabStop = false;
			// 
			// listBox1
			// 
			this->listBox1->FormattingEnabled = true;
			this->listBox1->HorizontalScrollbar = true;
			this->listBox1->Location = System::Drawing::Point(12, 342);
			this->listBox1->Name = L"listBox1";
			this->listBox1->Size = System::Drawing::Size(167, 186);
			this->listBox1->TabIndex = 1;
			this->listBox1->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::listBox1_SelectedIndexChanged);
			// 
			// listBox2
			// 
			this->listBox2->FormattingEnabled = true;
			this->listBox2->HorizontalScrollbar = true;
			this->listBox2->Location = System::Drawing::Point(805, 342);
			this->listBox2->Name = L"listBox2";
			this->listBox2->Size = System::Drawing::Size(167, 186);
			this->listBox2->TabIndex = 1;
			this->listBox2->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::listBox2_SelectedIndexChanged);
			// 
			// searchpat1
			// 
			this->searchpat1->Location = System::Drawing::Point(185, 363);
			this->searchpat1->Name = L"searchpat1";
			this->searchpat1->Size = System::Drawing::Size(173, 20);
			this->searchpat1->TabIndex = 2;
			this->searchpat1->Text = L"TOCS*.bmp";
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(243, 345);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(77, 13);
			this->label1->TabIndex = 3;
			this->label1->Text = L"Search pattern";
			// 
			// searchpat2
			// 
			this->searchpat2->Location = System::Drawing::Point(626, 361);
			this->searchpat2->Name = L"searchpat2";
			this->searchpat2->Size = System::Drawing::Size(173, 20);
			this->searchpat2->TabIndex = 2;
			this->searchpat2->Text = L"TBCS*.bmp";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(664, 345);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(77, 13);
			this->label2->TabIndex = 3;
			this->label2->Text = L"Search pattern";
			// 
			// search1
			// 
			this->search1->Location = System::Drawing::Point(185, 393);
			this->search1->Name = L"search1";
			this->search1->Size = System::Drawing::Size(103, 21);
			this->search1->TabIndex = 4;
			this->search1->Text = L"Search";
			this->search1->UseVisualStyleBackColor = true;
			this->search1->Click += gcnew System::EventHandler(this, &Form1::search1_Click);
			// 
			// search2
			// 
			this->search2->Location = System::Drawing::Point(696, 391);
			this->search2->Name = L"search2";
			this->search2->Size = System::Drawing::Size(103, 21);
			this->search2->TabIndex = 4;
			this->search2->Text = L"Search";
			this->search2->UseVisualStyleBackColor = true;
			this->search2->Click += gcnew System::EventHandler(this, &Form1::search2_Click);
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 18, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(238)));
			this->label3->Location = System::Drawing::Point(160, 6);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(81, 29);
			this->label3->TabIndex = 5;
			this->label3->Text = L"POLE";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 18, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(238)));
			this->label4->Location = System::Drawing::Point(743, 6);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(93, 29);
			this->label4->TabIndex = 5;
			this->label4->Text = L"IMAGE";
			// 
			// prev1
			// 
			this->prev1->Enabled = false;
			this->prev1->Location = System::Drawing::Point(294, 393);
			this->prev1->Name = L"prev1";
			this->prev1->Size = System::Drawing::Size(64, 21);
			this->prev1->TabIndex = 6;
			this->prev1->Text = L"<-";
			this->prev1->UseVisualStyleBackColor = true;
			this->prev1->Click += gcnew System::EventHandler(this, &Form1::prev1_Click);
			// 
			// next1
			// 
			this->next1->Enabled = false;
			this->next1->Location = System::Drawing::Point(364, 393);
			this->next1->Name = L"next1";
			this->next1->Size = System::Drawing::Size(64, 21);
			this->next1->TabIndex = 6;
			this->next1->Text = L"->";
			this->next1->UseVisualStyleBackColor = true;
			this->next1->Click += gcnew System::EventHandler(this, &Form1::next1_Click);
			// 
			// prev2
			// 
			this->prev2->Enabled = false;
			this->prev2->Location = System::Drawing::Point(556, 391);
			this->prev2->Name = L"prev2";
			this->prev2->Size = System::Drawing::Size(64, 21);
			this->prev2->TabIndex = 6;
			this->prev2->Text = L"<-";
			this->prev2->UseVisualStyleBackColor = true;
			this->prev2->Click += gcnew System::EventHandler(this, &Form1::prev2_Click);
			// 
			// next2
			// 
			this->next2->Enabled = false;
			this->next2->Location = System::Drawing::Point(626, 391);
			this->next2->Name = L"next2";
			this->next2->Size = System::Drawing::Size(64, 21);
			this->next2->TabIndex = 6;
			this->next2->Text = L"->";
			this->next2->UseVisualStyleBackColor = true;
			this->next2->Click += gcnew System::EventHandler(this, &Form1::next2_Click);
			// 
			// assign1
			// 
			this->assign1->Location = System::Drawing::Point(434, 391);
			this->assign1->Name = L"assign1";
			this->assign1->Size = System::Drawing::Size(116, 22);
			this->assign1->TabIndex = 7;
			this->assign1->Text = L"Assign there >";
			this->assign1->UseVisualStyleBackColor = true;
			this->assign1->Click += gcnew System::EventHandler(this, &Form1::assign1_Click);
			// 
			// assign2
			// 
			this->assign2->Location = System::Drawing::Point(434, 414);
			this->assign2->Name = L"assign2";
			this->assign2->Size = System::Drawing::Size(116, 22);
			this->assign2->TabIndex = 7;
			this->assign2->Text = L"< Assign there";
			this->assign2->UseVisualStyleBackColor = true;
			this->assign2->Click += gcnew System::EventHandler(this, &Form1::assign2_Click);
			// 
			// possibilites1
			// 
			this->possibilites1->FormattingEnabled = true;
			this->possibilites1->HorizontalScrollbar = true;
			this->possibilites1->Location = System::Drawing::Point(185, 422);
			this->possibilites1->Name = L"possibilites1";
			this->possibilites1->Size = System::Drawing::Size(120, 108);
			this->possibilites1->TabIndex = 8;
			this->possibilites1->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::possibilites1_SelectedIndexChanged);
			// 
			// possibilites2
			// 
			this->possibilites2->FormattingEnabled = true;
			this->possibilites2->HorizontalScrollbar = true;
			this->possibilites2->Location = System::Drawing::Point(679, 422);
			this->possibilites2->Name = L"possibilites2";
			this->possibilites2->Size = System::Drawing::Size(120, 108);
			this->possibilites2->TabIndex = 8;
			this->possibilites2->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::possibilites2_SelectedIndexChanged);
			// 
			// confirm1
			// 
			this->confirm1->Location = System::Drawing::Point(312, 422);
			this->confirm1->Name = L"confirm1";
			this->confirm1->Size = System::Drawing::Size(116, 25);
			this->confirm1->TabIndex = 9;
			this->confirm1->Text = L"Confirm";
			this->confirm1->UseVisualStyleBackColor = true;
			this->confirm1->Click += gcnew System::EventHandler(this, &Form1::confirm1_Click);
			// 
			// confirm2
			// 
			this->confirm2->Location = System::Drawing::Point(557, 422);
			this->confirm2->Name = L"confirm2";
			this->confirm2->Size = System::Drawing::Size(116, 25);
			this->confirm2->TabIndex = 9;
			this->confirm2->Text = L"Confirm";
			this->confirm2->UseVisualStyleBackColor = true;
			this->confirm2->Click += gcnew System::EventHandler(this, &Form1::confirm2_Click);
			// 
			// source1
			// 
			this->source1->Location = System::Drawing::Point(312, 468);
			this->source1->Name = L"source1";
			this->source1->Size = System::Drawing::Size(116, 20);
			this->source1->TabIndex = 10;
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(335, 452);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(73, 13);
			this->label5->TabIndex = 11;
			this->label5->Text = L"Source folder:";
			// 
			// source2
			// 
			this->source2->Location = System::Drawing::Point(556, 468);
			this->source2->Name = L"source2";
			this->source2->Size = System::Drawing::Size(116, 20);
			this->source2->TabIndex = 10;
			// 
			// label6
			// 
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(579, 452);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(73, 13);
			this->label6->TabIndex = 11;
			this->label6->Text = L"Source folder:";
			// 
			// browse1
			// 
			this->browse1->Location = System::Drawing::Point(312, 494);
			this->browse1->Name = L"browse1";
			this->browse1->Size = System::Drawing::Size(116, 23);
			this->browse1->TabIndex = 12;
			this->browse1->Text = L"Browse";
			this->browse1->UseVisualStyleBackColor = true;
			this->browse1->Click += gcnew System::EventHandler(this, &Form1::browse1_Click);
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(557, 494);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(116, 23);
			this->button1->TabIndex = 12;
			this->button1->Text = L"Browse";
			this->button1->UseVisualStyleBackColor = true;
			// 
			// browse2
			// 
			this->browse2->Location = System::Drawing::Point(556, 494);
			this->browse2->Name = L"browse2";
			this->browse2->Size = System::Drawing::Size(116, 23);
			this->browse2->TabIndex = 12;
			this->browse2->Text = L"Browse";
			this->browse2->UseVisualStyleBackColor = true;
			this->browse2->Click += gcnew System::EventHandler(this, &Form1::browse2_Click);
			// 
			// label7
			// 
			this->label7->AutoSize = true;
			this->label7->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(238)));
			this->label7->Location = System::Drawing::Point(440, 472);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(104, 20);
			this->label7->TabIndex = 13;
			this->label7->Text = L"Obwodziciel";
			// 
			// label8
			// 
			this->label8->AutoSize = true;
			this->label8->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(238)));
			this->label8->Location = System::Drawing::Point(442, 492);
			this->label8->Name = L"label8";
			this->label8->Size = System::Drawing::Size(102, 20);
			this->label8->TabIndex = 13;
			this->label8->Text = L"VCMI Team";
			// 
			// label9
			// 
			this->label9->AutoSize = true;
			this->label9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(238)));
			this->label9->Location = System::Drawing::Point(459, 512);
			this->label9->Name = L"label9";
			this->label9->Size = System::Drawing::Size(66, 20);
			this->label9->TabIndex = 13;
			this->label9->Text = L"© 2008";
			// 
			// label10
			// 
			this->label10->AutoSize = true;
			this->label10->Location = System::Drawing::Point(463, 344);
			this->label10->Name = L"label10";
			this->label10->Size = System::Drawing::Size(58, 13);
			this->label10->TabIndex = 16;
			this->label10->Text = L"Building ID";
			// 
			// buildingTypes
			// 
			this->buildingTypes->FormattingEnabled = true;
			this->buildingTypes->Location = System::Drawing::Point(365, 364);
			this->buildingTypes->Name = L"buildingTypes";
			this->buildingTypes->Size = System::Drawing::Size(255, 21);
			this->buildingTypes->TabIndex = 17;
			// 
			// castleType
			// 
			this->castleType->FormattingEnabled = true;
			this->castleType->Location = System::Drawing::Point(434, 445);
			this->castleType->Name = L"castleType";
			this->castleType->Size = System::Drawing::Size(116, 21);
			this->castleType->TabIndex = 18;
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(984, 535);
			this->Controls->Add(this->castleType);
			this->Controls->Add(this->buildingTypes);
			this->Controls->Add(this->label10);
			this->Controls->Add(this->label9);
			this->Controls->Add(this->label8);
			this->Controls->Add(this->label7);
			this->Controls->Add(this->browse2);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->browse1);
			this->Controls->Add(this->label6);
			this->Controls->Add(this->label5);
			this->Controls->Add(this->source2);
			this->Controls->Add(this->source1);
			this->Controls->Add(this->confirm2);
			this->Controls->Add(this->confirm1);
			this->Controls->Add(this->possibilites2);
			this->Controls->Add(this->possibilites1);
			this->Controls->Add(this->assign2);
			this->Controls->Add(this->assign1);
			this->Controls->Add(this->next2);
			this->Controls->Add(this->next1);
			this->Controls->Add(this->prev2);
			this->Controls->Add(this->prev1);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->search2);
			this->Controls->Add(this->search1);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->searchpat2);
			this->Controls->Add(this->searchpat1);
			this->Controls->Add(this->listBox2);
			this->Controls->Add(this->listBox1);
			this->Controls->Add(this->pictureBox2);
			this->Controls->Add(this->pictureBox1);
			this->Name = L"Form1";
			this->Text = L"Obwodziciel 1.00";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox1))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox2))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void browse1_Click(System::Object^  sender, System::EventArgs^  e) {
				 folderBrowserDialog1->ShowDialog();
				 source1->Text = folderBrowserDialog1->SelectedPath;
			 }
private: System::Void browse2_Click(System::Object^  sender, System::EventArgs^  e) {
				 folderBrowserDialog1->ShowDialog();
				 source2->Text = folderBrowserDialog1->SelectedPath;
		 }
private: System::Void search1_Click(System::Object^  sender, System::EventArgs^  e) {
			 listBox1->Items->Clear();
			 array<String^>^ pliki = IO::Directory::GetFiles(source1->Text,searchpat1->Text);
			 for each(String^ plik in pliki)
			 {
				 listBox1->Items->Add(plik);
			 }
			if(listBox1->Items->Count>0)
				listBox1->SelectedIndex=0;
		 }
private: System::Void search2_Click(System::Object^  sender, System::EventArgs^  e) {
			 listBox2->Items->Clear();
			 array<String^>^ pliki = IO::Directory::GetFiles(source2->Text,searchpat2->Text);
			 for each(String^ plik in pliki)
			 {
				 listBox2->Items->Add(plik);
			 }
			if(listBox2->Items->Count>0)
				listBox2->SelectedIndex=0;
		 }
private: System::Void listBox1_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(listBox1->SelectedIndex<0)
				 return;
			 pictureBox1->Image = gcnew Drawing::Bitmap(dynamic_cast<String^>(listBox1->SelectedItem));
		 }
private: System::Void listBox2_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(listBox2->SelectedIndex<0)
				 return;
			 String^ s = dynamic_cast<String^>(listBox2->SelectedItem);
			 pictureBox2->Image = gcnew Drawing::Bitmap(s);
			 s = s->Substring(s->LastIndexOf(L"\\")+1);
			 s = s->Replace(L"_0_",L"");
			 s = s->Replace(L".bmp",L".def");
			 s = s->Replace(L".BMP",L".def");
			 for each(BuildingEntry b in lista)
			 {
				 if(b.defname == s)
					 buildingTypes->SelectedIndex = b.ID;
			 }
		 }
private: System::Void assign2_Click(System::Object^  sender, System::EventArgs^  e) {
			 possibilites1->Items->Clear();
			 for each(Object^ item in listBox1->Items)
			 {
				 String^ path = dynamic_cast<String^>(item);
				 Bitmap ^b = gcnew Bitmap(path);
				 if( (b->Width == pictureBox2->Image->Width) && (b->Height == pictureBox2->Image->Height) )
					 possibilites1->Items->Add(path);
				 delete b;
			 }
			 if (possibilites1->Items->Count>0)
			 {
				 possibilites1->SelectedIndex=0;
			 }
		 }
private: System::Void assign1_Click(System::Object^  sender, System::EventArgs^  e) {
			 possibilites2->Items->Clear();
			 for each(Object^ item in listBox2->Items)
			 {
				 String^ path = dynamic_cast<String^>(item);
				 Bitmap ^b = gcnew Bitmap(path);
				 if( (b->Width == pictureBox1->Image->Width) && (b->Height == pictureBox1->Image->Height) )
					 possibilites2->Items->Add(path);
				 delete b;
			 }
			 if (possibilites2->Items->Count>0)
			 {
				 possibilites2->SelectedIndex=0;
			 }
		 }
private: System::Void possibilites1_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(possibilites1->SelectedIndex<0)
				 return;
			 String^ s = dynamic_cast<String^>(possibilites1->SelectedItem);
			 pictureBox1->Image = gcnew Drawing::Bitmap(s);
			 s = s->Substring(s->LastIndexOf(L"\\")+1);
			 s = s->Replace(L"_0_",L"");
			 s = s->Replace(L".bmp",L".def");
			 s = s->Replace(L".BMP",L".def");
			 for each(BuildingEntry b in lista)
			 {
				 if(b.defname == s)
					 buildingTypes->SelectedIndex = b.ID;
			 }
		 }
private: System::Void possibilites2_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(possibilites2->SelectedIndex<0)
				 return;
			 String^ s = dynamic_cast<String^>(possibilites2->SelectedItem);
			 pictureBox2->Image = gcnew Drawing::Bitmap(s);
			 s = s->Substring(s->LastIndexOf(L"\\")+1);
			 s = s->Replace(L"_0_",L"");
			 s = s->Replace(L".bmp",L".def");
			 s = s->Replace(L".BMP",L".def");
			 for each(BuildingEntry b in lista)
			 {
				 if(b.defname == s)
					 buildingTypes->SelectedIndex = b.ID;
			 }
		 }
		 void confirm(bool first)
		 {
			 try
			 {
				 //out format: 
				 //	townId	ID	defname	bordername	areaname

				 String^ s, ^s2, ^s3;
				 if(first)
				 {
					s = dynamic_cast<String^>(possibilites2->SelectedItem);
				 }
				 else
				 {
					 s = dynamic_cast<String^>(listBox2->SelectedItem);
				 }
				 s = s->Substring(s->LastIndexOf(L"\\")+1);
				 s = s->Replace(L"_0_",L"");
				 s = s->Replace(L".bmp",L".def");
				 s = s->Replace(L".BMP",L".def");

				 if(!first)
				 {
					 s2 = dynamic_cast<String^>(possibilites1->SelectedItem);
				 }
				 else
				 {
					 s2 = dynamic_cast<String^>(listBox1->SelectedItem);
				 }
				 s2 = s2->Substring(s2->LastIndexOf(L"\\")+1);

				 s3 = L"TZ" + s2->Substring(2);

				 String^ toOut;
				 toOut = castleType->SelectedIndex.ToString() + L"\t" 
							+ buildingTypes->SelectedIndex + (L"\t") 
							+ s + L"\t" 
							+ s2 +  L"\t"
							+ s3;
				 out->WriteLine(toOut);
				 out->Flush();

				 if(first)
				 {
					 for each(Object^ obj in listBox2->Items)
					 {
						String^ str = dynamic_cast<String^>(obj),
							^usw = dynamic_cast<String^>(possibilites2->SelectedItem);
						if(str==usw)
						{
							listBox2->Items->Remove(obj);
							if(listBox2->Items->Count>0)
								listBox2->SelectedIndex = 0;
							break;
						}
					 }
					 listBox1->Items->Remove(listBox1->SelectedItem);
					 if(listBox1->Items->Count>0)
						 listBox1->SelectedIndex++;
				 }
				 else
				 {
					 for each(Object^ obj in listBox1->Items)
					 {
						String^ str = dynamic_cast<String^>(obj),
							^usw = dynamic_cast<String^>(possibilites1->SelectedItem);
						if(str==usw)
						{
							listBox1->Items->Remove(obj);
							if(listBox1->Items->Count>0)
								listBox1->SelectedIndex = 0;
							break;
						}
					 }
					 int tempp = listBox2->SelectedIndex;
					 listBox2->Items->Remove(listBox2->SelectedItem);
					 if(listBox2->Items->Count>tempp)
						 listBox2->SelectedIndex=tempp;
				 }

				 possibilites1->Items->Clear();
				 possibilites2->Items->Clear();
			 }
			 catch(...)
			 {
				 possibilites1->Items->Clear();
				 possibilites2->Items->Clear();
			 }
		 }
private: System::Void confirm1_Click(System::Object^  sender, System::EventArgs^  e) {
			confirm(true);
		 }
private: System::Void confirm2_Click(System::Object^  sender, System::EventArgs^  e) {
			confirm(false);
		 }
private: System::Void prev1_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void next1_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void prev2_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void next2_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
};
}

