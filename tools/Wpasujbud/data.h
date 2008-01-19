#pragma once
public ref class CBuildingData
{
public:
	System::String^ defname;
	System::Int32 ID, x, y;
	System::Int32 townID;
	virtual System::String^ ToString()override
	{
		return townID.ToString() + L" " + ID.ToString() + L" " +  defname + L" " +  x.ToString() + L" " +  y.ToString() + L"\n";
	}
};  