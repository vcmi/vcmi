#pragma once
#include <cliext/vector> 
#include "data.h"
#include "obrazek.h"
#include "dataEditor.h"
#include <string>
namespace Wpasuj {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;


	/// <summary>
	/// Summary for Wpasujbud
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Wpasujbud : public System::Windows::Forms::Form
	{
	public:
		cliext::vector<CBuildingData^> data;
		
		Int32 mx, my, curx, cury;
		SDL_Surface * sbg;
	private: System::Windows::Forms::TextBox^  foundedCoords;
	public: 

	public: 

	public: 

	public: 
	private: System::Windows::Forms::Label^  label11;
	private: System::Windows::Forms::Button^  confirm;

	private: System::Windows::Forms::Label^  label12;
	private: System::Windows::Forms::Button^  button3;
	private: System::Windows::Forms::TextBox^  accordanceBox;
	private: System::Windows::Forms::Button^  button4;
	private: System::Windows::Forms::TextBox^  inaccuracy;
	private: System::Windows::Forms::Label^  label13;
	private: System::Windows::Forms::Button^  save;

	private: System::Windows::Forms::RadioButton^  radioButton3;
	private: System::Windows::Forms::Button^  dataview;
	private: System::Windows::Forms::Label^  label9;
	private: System::Windows::Forms::Label^  label14;

	private: System::Windows::Forms::Button^  button1;
	public: 
	
		void searchPicturePos();
		void nextPicture();
		void previousPicture();
		std::string ToString(System::String^ src);


		obrazek ^curob;
		dataEditor ^cured;

		Wpasujbud(void);

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Wpasujbud()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::PictureBox^  townBg;
	private: System::Windows::Forms::PictureBox^  buildingImg;

	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::RadioButton^  radioButton1;
	private: System::Windows::Forms::RadioButton^  radioButton2;
	private: System::Windows::Forms::TextBox^  koordy;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::ComboBox^  buildingType;

	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::ComboBox^  townID;
	private: System::Windows::Forms::TextBox^  bmpPattern;
	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::ListBox^  bitmapList;
	private: System::Windows::Forms::Label^  label6;
	private: System::Windows::Forms::Button^  searchBitmaps;
	private: System::Windows::Forms::Button^  startLocating;

	private: System::Windows::Forms::TextBox^  townBgPath;
	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::Button^  setBackground;

	private: System::Windows::Forms::TextBox^  radious;
	private: System::Windows::Forms::Label^  label8;


	private: System::Windows::Forms::OpenFileDialog^  openFileDialog1;
	private: System::Windows::Forms::Button^  browseForBg;
	private: System::Windows::Forms::TextBox^  bitmapsFolder;
	private: System::Windows::Forms::Label^  label10;
	private: System::Windows::Forms::Button^  browseForbmpfol;

	private: System::Windows::Forms::FolderBrowserDialog^  folderBrowserDialog1;


	protected: 

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
		void InitializeComponent(void)
		{
			this->townBg = (gcnew System::Windows::Forms::PictureBox());
			this->buildingImg = (gcnew System::Windows::Forms::PictureBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->radioButton1 = (gcnew System::Windows::Forms::RadioButton());
			this->radioButton2 = (gcnew System::Windows::Forms::RadioButton());
			this->koordy = (gcnew System::Windows::Forms::TextBox());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->buildingType = (gcnew System::Windows::Forms::ComboBox());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->townID = (gcnew System::Windows::Forms::ComboBox());
			this->bmpPattern = (gcnew System::Windows::Forms::TextBox());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->bitmapList = (gcnew System::Windows::Forms::ListBox());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->searchBitmaps = (gcnew System::Windows::Forms::Button());
			this->startLocating = (gcnew System::Windows::Forms::Button());
			this->townBgPath = (gcnew System::Windows::Forms::TextBox());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->setBackground = (gcnew System::Windows::Forms::Button());
			this->radious = (gcnew System::Windows::Forms::TextBox());
			this->label8 = (gcnew System::Windows::Forms::Label());
			this->openFileDialog1 = (gcnew System::Windows::Forms::OpenFileDialog());
			this->browseForBg = (gcnew System::Windows::Forms::Button());
			this->bitmapsFolder = (gcnew System::Windows::Forms::TextBox());
			this->label10 = (gcnew System::Windows::Forms::Label());
			this->browseForbmpfol = (gcnew System::Windows::Forms::Button());
			this->folderBrowserDialog1 = (gcnew System::Windows::Forms::FolderBrowserDialog());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->foundedCoords = (gcnew System::Windows::Forms::TextBox());
			this->label11 = (gcnew System::Windows::Forms::Label());
			this->confirm = (gcnew System::Windows::Forms::Button());
			this->label12 = (gcnew System::Windows::Forms::Label());
			this->button3 = (gcnew System::Windows::Forms::Button());
			this->accordanceBox = (gcnew System::Windows::Forms::TextBox());
			this->button4 = (gcnew System::Windows::Forms::Button());
			this->inaccuracy = (gcnew System::Windows::Forms::TextBox());
			this->label13 = (gcnew System::Windows::Forms::Label());
			this->save = (gcnew System::Windows::Forms::Button());
			this->radioButton3 = (gcnew System::Windows::Forms::RadioButton());
			this->dataview = (gcnew System::Windows::Forms::Button());
			this->label9 = (gcnew System::Windows::Forms::Label());
			this->label14 = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->townBg))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->buildingImg))->BeginInit();
			this->SuspendLayout();
			// 
			// townBg
			// 
			this->townBg->Location = System::Drawing::Point(12, 6);
			this->townBg->Name = L"townBg";
			this->townBg->Size = System::Drawing::Size(800, 374);
			this->townBg->TabIndex = 0;
			this->townBg->TabStop = false;
			this->townBg->Click += gcnew System::EventHandler(this, &Wpasujbud::townBg_Click);
			// 
			// buildingImg
			// 
			this->buildingImg->Location = System::Drawing::Point(12, 401);
			this->buildingImg->Name = L"buildingImg";
			this->buildingImg->Size = System::Drawing::Size(163, 137);
			this->buildingImg->TabIndex = 1;
			this->buildingImg->TabStop = false;
			this->buildingImg->Click += gcnew System::EventHandler(this, &Wpasujbud::buildingImg_Click);
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(12, 385);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(166, 13);
			this->label1->TabIndex = 2;
			this->label1->Text = L"Current building (click for full size):";
			// 
			// radioButton1
			// 
			this->radioButton1->AutoSize = true;
			this->radioButton1->Checked = true;
			this->radioButton1->Location = System::Drawing::Point(185, 491);
			this->radioButton1->Name = L"radioButton1";
			this->radioButton1->Size = System::Drawing::Size(94, 17);
			this->radioButton1->TabIndex = 3;
			this->radioButton1->TabStop = true;
			this->radioButton1->Text = L"Top left corner";
			this->radioButton1->UseVisualStyleBackColor = true;
			// 
			// radioButton2
			// 
			this->radioButton2->AutoSize = true;
			this->radioButton2->Location = System::Drawing::Point(185, 512);
			this->radioButton2->Name = L"radioButton2";
			this->radioButton2->Size = System::Drawing::Size(85, 17);
			this->radioButton2->TabIndex = 3;
			this->radioButton2->Text = L"Middle of pic";
			this->radioButton2->UseVisualStyleBackColor = true;
			// 
			// koordy
			// 
			this->koordy->Location = System::Drawing::Point(185, 470);
			this->koordy->Name = L"koordy";
			this->koordy->Size = System::Drawing::Size(121, 20);
			this->koordy->TabIndex = 4;
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(214, 456);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(67, 13);
			this->label2->TabIndex = 5;
			this->label2->Text = L"Your coords:";
			// 
			// buildingType
			// 
			this->buildingType->FormattingEnabled = true;
			this->buildingType->Location = System::Drawing::Point(185, 400);
			this->buildingType->Name = L"buildingType";
			this->buildingType->Size = System::Drawing::Size(121, 21);
			this->buildingType->TabIndex = 6;
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(214, 385);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(67, 13);
			this->label3->TabIndex = 7;
			this->label3->Text = L"Building type";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(224, 422);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(48, 13);
			this->label4->TabIndex = 7;
			this->label4->Text = L"Town ID";
			// 
			// townID
			// 
			this->townID->FormattingEnabled = true;
			this->townID->Location = System::Drawing::Point(185, 435);
			this->townID->Name = L"townID";
			this->townID->Size = System::Drawing::Size(121, 21);
			this->townID->TabIndex = 6;
			// 
			// bmpPattern
			// 
			this->bmpPattern->Location = System::Drawing::Point(317, 467);
			this->bmpPattern->Name = L"bmpPattern";
			this->bmpPattern->Size = System::Drawing::Size(121, 20);
			this->bmpPattern->TabIndex = 4;
			this->bmpPattern->Text = L"*.bmp";
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(332, 452);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(103, 13);
			this->label5->TabIndex = 5;
			this->label5->Text = L"Search bmp pattern:";
			// 
			// bitmapList
			// 
			this->bitmapList->Enabled = false;
			this->bitmapList->FormattingEnabled = true;
			this->bitmapList->Location = System::Drawing::Point(449, 400);
			this->bitmapList->Name = L"bitmapList";
			this->bitmapList->Size = System::Drawing::Size(133, 95);
			this->bitmapList->TabIndex = 8;
			this->bitmapList->SelectedIndexChanged += gcnew System::EventHandler(this, &Wpasujbud::bitmapList_SelectedIndexChanged);
			// 
			// label6
			// 
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(485, 384);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(57, 13);
			this->label6->TabIndex = 9;
			this->label6->Text = L"Bitmap list:";
			// 
			// searchBitmaps
			// 
			this->searchBitmaps->Location = System::Drawing::Point(449, 501);
			this->searchBitmaps->Name = L"searchBitmaps";
			this->searchBitmaps->Size = System::Drawing::Size(133, 23);
			this->searchBitmaps->TabIndex = 10;
			this->searchBitmaps->Text = L"Search bitmaps";
			this->searchBitmaps->UseVisualStyleBackColor = true;
			this->searchBitmaps->Click += gcnew System::EventHandler(this, &Wpasujbud::searchBitmaps_Click);
			// 
			// startLocating
			// 
			this->startLocating->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(238)));
			this->startLocating->Location = System::Drawing::Point(696, 464);
			this->startLocating->Name = L"startLocating";
			this->startLocating->Size = System::Drawing::Size(120, 29);
			this->startLocating->TabIndex = 11;
			this->startLocating->Text = L"Search pos!";
			this->startLocating->UseVisualStyleBackColor = true;
			this->startLocating->Click += gcnew System::EventHandler(this, &Wpasujbud::startLocating_Click);
			// 
			// townBgPath
			// 
			this->townBgPath->Location = System::Drawing::Point(317, 403);
			this->townBgPath->Name = L"townBgPath";
			this->townBgPath->Size = System::Drawing::Size(121, 20);
			this->townBgPath->TabIndex = 4;
			// 
			// label7
			// 
			this->label7->AutoSize = true;
			this->label7->Location = System::Drawing::Point(332, 386);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(94, 13);
			this->label7->TabIndex = 5;
			this->label7->Text = L"Town background";
			// 
			// setBackground
			// 
			this->setBackground->Location = System::Drawing::Point(317, 427);
			this->setBackground->Name = L"setBackground";
			this->setBackground->Size = System::Drawing::Size(52, 23);
			this->setBackground->TabIndex = 12;
			this->setBackground->Text = L"Set bg";
			this->setBackground->UseVisualStyleBackColor = true;
			this->setBackground->Click += gcnew System::EventHandler(this, &Wpasujbud::setBackground_Click);
			// 
			// radious
			// 
			this->radious->Location = System::Drawing::Point(591, 441);
			this->radious->Name = L"radious";
			this->radious->Size = System::Drawing::Size(98, 20);
			this->radious->TabIndex = 13;
			this->radious->Text = L"100";
			// 
			// label8
			// 
			this->label8->AutoSize = true;
			this->label8->Location = System::Drawing::Point(597, 426);
			this->label8->Name = L"label8";
			this->label8->Size = System::Drawing::Size(62, 13);
			this->label8->TabIndex = 5;
			this->label8->Text = L"Search rad:";
			// 
			// openFileDialog1
			// 
			this->openFileDialog1->FileName = L"openFileDialog1";
			this->openFileDialog1->Filter = L"BMP Files (*.bmp)|*.bmp|All files (*.*)|*.*";
			this->openFileDialog1->FileOk += gcnew System::ComponentModel::CancelEventHandler(this, &Wpasujbud::openFileDialog1_FileOk);
			// 
			// browseForBg
			// 
			this->browseForBg->Location = System::Drawing::Point(373, 427);
			this->browseForBg->Name = L"browseForBg";
			this->browseForBg->Size = System::Drawing::Size(65, 23);
			this->browseForBg->TabIndex = 14;
			this->browseForBg->Text = L"Browse";
			this->browseForBg->UseVisualStyleBackColor = true;
			this->browseForBg->Click += gcnew System::EventHandler(this, &Wpasujbud::browseForBg_Click);
			// 
			// bitmapsFolder
			// 
			this->bitmapsFolder->Location = System::Drawing::Point(317, 505);
			this->bitmapsFolder->Name = L"bitmapsFolder";
			this->bitmapsFolder->Size = System::Drawing::Size(121, 20);
			this->bitmapsFolder->TabIndex = 4;
			// 
			// label10
			// 
			this->label10->AutoSize = true;
			this->label10->Location = System::Drawing::Point(334, 490);
			this->label10->Name = L"label10";
			this->label10->Size = System::Drawing::Size(97, 13);
			this->label10->TabIndex = 5;
			this->label10->Text = L"Folder with bitmaps";
			// 
			// browseForbmpfol
			// 
			this->browseForbmpfol->Location = System::Drawing::Point(321, 528);
			this->browseForbmpfol->Name = L"browseForbmpfol";
			this->browseForbmpfol->Size = System::Drawing::Size(111, 19);
			this->browseForbmpfol->TabIndex = 15;
			this->browseForbmpfol->Text = L"Browse";
			this->browseForbmpfol->UseVisualStyleBackColor = true;
			this->browseForbmpfol->Click += gcnew System::EventHandler(this, &Wpasujbud::browseForbmpfol_Click);
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(592, 466);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(97, 23);
			this->button1->TabIndex = 16;
			this->button1->Text = L"Next pic";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Wpasujbud::button1_Click);
			// 
			// foundedCoords
			// 
			this->foundedCoords->Location = System::Drawing::Point(696, 441);
			this->foundedCoords->Name = L"foundedCoords";
			this->foundedCoords->Size = System::Drawing::Size(120, 20);
			this->foundedCoords->TabIndex = 17;
			this->foundedCoords->Text = L"0, 0";
			// 
			// label11
			// 
			this->label11->AutoSize = true;
			this->label11->Location = System::Drawing::Point(722, 425);
			this->label11->Name = L"label11";
			this->label11->Size = System::Drawing::Size(87, 13);
			this->label11->TabIndex = 18;
			this->label11->Text = L"Founded coords:";
			// 
			// confirm
			// 
			this->confirm->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(238)));
			this->confirm->Location = System::Drawing::Point(590, 525);
			this->confirm->Name = L"confirm";
			this->confirm->Size = System::Drawing::Size(120, 23);
			this->confirm->TabIndex = 19;
			this->confirm->Text = L"Confirm";
			this->confirm->UseVisualStyleBackColor = true;
			this->confirm->Click += gcnew System::EventHandler(this, &Wpasujbud::confirm_Click);
			// 
			// label12
			// 
			this->label12->AutoSize = true;
			this->label12->Location = System::Drawing::Point(722, 385);
			this->label12->Name = L"label12";
			this->label12->Size = System::Drawing::Size(68, 13);
			this->label12->TabIndex = 21;
			this->label12->Text = L"Accordance:";
			// 
			// button3
			// 
			this->button3->Location = System::Drawing::Point(736, 496);
			this->button3->Name = L"button3";
			this->button3->Size = System::Drawing::Size(83, 23);
			this->button3->TabIndex = 22;
			this->button3->Text = L"Check pos";
			this->button3->UseVisualStyleBackColor = true;
			// 
			// accordanceBox
			// 
			this->accordanceBox->Enabled = false;
			this->accordanceBox->Location = System::Drawing::Point(696, 401);
			this->accordanceBox->Name = L"accordanceBox";
			this->accordanceBox->Size = System::Drawing::Size(115, 20);
			this->accordanceBox->TabIndex = 20;
			this->accordanceBox->Text = L"0/0";
			// 
			// button4
			// 
			this->button4->Location = System::Drawing::Point(592, 496);
			this->button4->Name = L"button4";
			this->button4->Size = System::Drawing::Size(83, 23);
			this->button4->TabIndex = 23;
			this->button4->Text = L"Previous pic";
			this->button4->UseVisualStyleBackColor = true;
			this->button4->Click += gcnew System::EventHandler(this, &Wpasujbud::button4_Click);
			// 
			// inaccuracy
			// 
			this->inaccuracy->Location = System::Drawing::Point(592, 401);
			this->inaccuracy->Name = L"inaccuracy";
			this->inaccuracy->Size = System::Drawing::Size(98, 20);
			this->inaccuracy->TabIndex = 20;
			this->inaccuracy->Text = L"5";
			this->inaccuracy->TextChanged += gcnew System::EventHandler(this, &Wpasujbud::inaccuracy_TextChanged);
			// 
			// label13
			// 
			this->label13->AutoSize = true;
			this->label13->Location = System::Drawing::Point(587, 385);
			this->label13->Name = L"label13";
			this->label13->Size = System::Drawing::Size(88, 13);
			this->label13->TabIndex = 21;
			this->label13->Text = L"Inaccuracy level:";
			// 
			// save
			// 
			this->save->Location = System::Drawing::Point(681, 497);
			this->save->Name = L"save";
			this->save->Size = System::Drawing::Size(48, 23);
			this->save->TabIndex = 24;
			this->save->Text = L"Save";
			this->save->UseVisualStyleBackColor = true;
			this->save->Click += gcnew System::EventHandler(this, &Wpasujbud::save_Click);
			// 
			// radioButton3
			// 
			this->radioButton3->AutoSize = true;
			this->radioButton3->Location = System::Drawing::Point(185, 531);
			this->radioButton3->Name = L"radioButton3";
			this->radioButton3->Size = System::Drawing::Size(114, 17);
			this->radioButton3->TabIndex = 3;
			this->radioButton3->Text = L"Bottom right corner";
			this->radioButton3->UseVisualStyleBackColor = true;
			// 
			// dataview
			// 
			this->dataview->Location = System::Drawing::Point(449, 528);
			this->dataview->Name = L"dataview";
			this->dataview->Size = System::Drawing::Size(135, 21);
			this->dataview->TabIndex = 25;
			this->dataview->Text = L"Data view/edition";
			this->dataview->UseVisualStyleBackColor = true;
			this->dataview->Click += gcnew System::EventHandler(this, &Wpasujbud::dataview_Click);
			// 
			// label9
			// 
			this->label9->AutoSize = true;
			this->label9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(238)));
			this->label9->Location = System::Drawing::Point(722, 523);
			this->label9->Name = L"label9";
			this->label9->Size = System::Drawing::Size(99, 15);
			this->label9->TabIndex = 26;
			this->label9->Text = L"By VCMI Team";
			// 
			// label14
			// 
			this->label14->AutoSize = true;
			this->label14->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(238)));
			this->label14->Location = System::Drawing::Point(742, 538);
			this->label14->Name = L"label14";
			this->label14->Size = System::Drawing::Size(53, 15);
			this->label14->TabIndex = 27;
			this->label14->Text = L"© 2008";
			// 
			// Wpasujbud
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(823, 554);
			this->Controls->Add(this->label14);
			this->Controls->Add(this->label9);
			this->Controls->Add(this->dataview);
			this->Controls->Add(this->save);
			this->Controls->Add(this->button4);
			this->Controls->Add(this->button3);
			this->Controls->Add(this->label13);
			this->Controls->Add(this->label12);
			this->Controls->Add(this->inaccuracy);
			this->Controls->Add(this->accordanceBox);
			this->Controls->Add(this->confirm);
			this->Controls->Add(this->label11);
			this->Controls->Add(this->foundedCoords);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->browseForbmpfol);
			this->Controls->Add(this->browseForBg);
			this->Controls->Add(this->radious);
			this->Controls->Add(this->setBackground);
			this->Controls->Add(this->startLocating);
			this->Controls->Add(this->searchBitmaps);
			this->Controls->Add(this->label6);
			this->Controls->Add(this->bitmapList);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->townID);
			this->Controls->Add(this->buildingType);
			this->Controls->Add(this->label8);
			this->Controls->Add(this->label7);
			this->Controls->Add(this->label10);
			this->Controls->Add(this->label5);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->bitmapsFolder);
			this->Controls->Add(this->townBgPath);
			this->Controls->Add(this->bmpPattern);
			this->Controls->Add(this->koordy);
			this->Controls->Add(this->radioButton3);
			this->Controls->Add(this->radioButton2);
			this->Controls->Add(this->radioButton1);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->buildingImg);
			this->Controls->Add(this->townBg);
			this->MaximizeBox = false;
			this->Name = L"Wpasujbud";
			this->Text = L"Cudowny wpasowywacz 1.00";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->townBg))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->buildingImg))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: 
		System::Void searchBitmaps_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void startLocating_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void setBackground_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void openFileDialog1_FileOk(System::Object^  sender, System::ComponentModel::CancelEventArgs^  e) ;
		System::Void browseForBg_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void browseForbmpfol_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void bitmapList_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
		System::Void townBg_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void button1_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void inaccuracy_TextChanged(System::Object^  sender, System::EventArgs^  e);
		System::Void confirm_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void button4_Click(System::Object^  sender, System::EventArgs^  e) {
			 previousPicture();
		 }
		System::Void save_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void buildingImg_Click(System::Object^  sender, System::EventArgs^  e);
		System::Void dataview_Click(System::Object^  sender, System::EventArgs^  e);

};

}

