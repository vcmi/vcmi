#pragma once
#include "data.h"
#include <cliext/vector> 
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace Wpasuj {

	/// <summary>
	/// Summary for dataEditor
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class dataEditor : public System::Windows::Forms::Form
	{
	public:
		dataEditor(cliext::vector<CBuildingData^> ^Data)
		{
			data = Data;
			InitializeComponent();
			dataGridView1->Rows->Add(data->size());
			for (int i=0;i<data->size();i++)
			{
				dataGridView1->Rows[i]->Cells[0]->Value = data->at(i)->townID;
				dataGridView1->Rows[i]->Cells[1]->Value = data->at(i)->ID;
				dataGridView1->Rows[i]->Cells[2]->Value = data->at(i)->defname;
				dataGridView1->Rows[i]->Cells[3]->Value = data->at(i)->x;
				dataGridView1->Rows[i]->Cells[4]->Value = data->at(i)->y;
			}
		}
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  townid;
	public: 
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  buildingID;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  defname;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  x;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  y;
	private: System::Windows::Forms::Button^  save;
	private: System::Windows::Forms::Button^  clear;
	private: System::Windows::Forms::Button^  close;
	private:

		cliext::vector<CBuildingData^> ^data;
	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~dataEditor()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::DataGridView^  dataGridView1;





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
			this->dataGridView1 = (gcnew System::Windows::Forms::DataGridView());
			this->townid = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->buildingID = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->defname = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->x = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->y = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->save = (gcnew System::Windows::Forms::Button());
			this->clear = (gcnew System::Windows::Forms::Button());
			this->close = (gcnew System::Windows::Forms::Button());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->dataGridView1))->BeginInit();
			this->SuspendLayout();
			// 
			// dataGridView1
			// 
			this->dataGridView1->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->dataGridView1->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(5) {this->townid, 
				this->buildingID, this->defname, this->x, this->y});
			this->dataGridView1->Location = System::Drawing::Point(12, 12);
			this->dataGridView1->Name = L"dataGridView1";
			this->dataGridView1->Size = System::Drawing::Size(334, 276);
			this->dataGridView1->TabIndex = 0;
			// 
			// townid
			// 
			this->townid->HeaderText = L"Town ID";
			this->townid->Name = L"townid";
			this->townid->Width = 50;
			// 
			// buildingID
			// 
			this->buildingID->HeaderText = L"Building ID";
			this->buildingID->Name = L"buildingID";
			this->buildingID->Width = 60;
			// 
			// defname
			// 
			this->defname->HeaderText = L"Def name";
			this->defname->Name = L"defname";
			this->defname->Width = 120;
			// 
			// x
			// 
			this->x->HeaderText = L"X";
			this->x->Name = L"x";
			this->x->Width = 30;
			// 
			// y
			// 
			this->y->HeaderText = L"Y";
			this->y->Name = L"y";
			this->y->Width = 30;
			// 
			// save
			// 
			this->save->Location = System::Drawing::Point(12, 294);
			this->save->Name = L"save";
			this->save->Size = System::Drawing::Size(105, 23);
			this->save->TabIndex = 1;
			this->save->Text = L"Save changes";
			this->save->UseVisualStyleBackColor = true;
			this->save->Click += gcnew System::EventHandler(this, &dataEditor::save_Click);
			// 
			// clear
			// 
			this->clear->Location = System::Drawing::Point(248, 294);
			this->clear->Name = L"clear";
			this->clear->Size = System::Drawing::Size(96, 23);
			this->clear->TabIndex = 2;
			this->clear->Text = L"Clear changes";
			this->clear->UseVisualStyleBackColor = true;
			this->clear->Click += gcnew System::EventHandler(this, &dataEditor::clear_Click);
			// 
			// close
			// 
			this->close->Location = System::Drawing::Point(123, 294);
			this->close->Name = L"close";
			this->close->Size = System::Drawing::Size(119, 23);
			this->close->TabIndex = 3;
			this->close->Text = L"Close";
			this->close->UseVisualStyleBackColor = true;
			this->close->Click += gcnew System::EventHandler(this, &dataEditor::close_Click);
			// 
			// dataEditor
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(356, 328);
			this->Controls->Add(this->close);
			this->Controls->Add(this->clear);
			this->Controls->Add(this->save);
			this->Controls->Add(this->dataGridView1);
			this->Name = L"dataEditor";
			this->Text = L"dataEditor";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->dataGridView1))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion

	private: System::Void clear_Click(System::Object^  sender, System::EventArgs^  e) {
				for (int i=0;i<data->size();i++)
				{
					dataGridView1->Rows[i]->Cells[0]->Value = data->at(i)->townID;
					dataGridView1->Rows[i]->Cells[1]->Value = data->at(i)->ID;
					dataGridView1->Rows[i]->Cells[2]->Value = data->at(i)->defname;
					dataGridView1->Rows[i]->Cells[3]->Value = data->at(i)->x;
					dataGridView1->Rows[i]->Cells[4]->Value = data->at(i)->y;
				}
			}
private: System::Void close_Click(System::Object^  sender, System::EventArgs^  e) {
			 Close();
		 }
private: System::Void save_Click(System::Object^  sender, System::EventArgs^  e) {
				for (int i=0;i<data->size();i++)
				{
					data->at(i)->townID =  Convert::ToInt32( static_cast<String^>(dataGridView1->Rows[i]->Cells[0]->Value));
					data->at(i)->ID = Convert::ToInt32( static_cast<String^>(dataGridView1->Rows[i]->Cells[1]->Value));
					data->at(i)->defname =  static_cast<String^>(dataGridView1->Rows[i]->Cells[2]->Value);
					data->at(i)->x = Convert::ToInt32( static_cast<String^>(dataGridView1->Rows[i]->Cells[3]->Value));
					data->at(i)->y = Convert::ToInt32( static_cast<String^>(dataGridView1->Rows[i]->Cells[4]->Value));
				}
		 }
};
}
