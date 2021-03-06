#include "xcpldapp.h"

#define IMAGEFILE <xcpldapp/xcpldapp.iml>
#include <Draw/iml_source.h>



xcpldapp::xcpldapp()
{
	CtrlLayout(*this, "XMOS Slice kit configurator");
	dlMBoard.Add("XP-SKC-L2");
	dlSBoard.Add("XP-SKC-L2");
	dlSBoard.Add("XA-SK-E100");
	dlSBoard.Add("XA-SK-AUDIO");
	dlSBoard.Add("XA-SK-SCR480");
	dlSBoard.Add("XA-SK-GPIO");
	dlSBoard.Add("XA-SK-SDRAM");
	dlSBoard.Add("XK-SK-ISBUS");
	dlSBoard.Add("XK-SK-WIFI");
	//XA_SK_WIFI
	dlMConn.Add("Star").Add("Triangle").Add("Square").Add("Circle");
	dlMBoard.WhenAction= THISBACK(Dlmboard);
	dlSBoard.WhenAction= THISBACK(Dlsboard);
	dlMConn.WhenAction= THISBACK(Dlmconn);
	aPins.WhenAction=THISBACK(Apins);
	bnVerilog.WhenAction= THISBACK(Bnverilog);
	Sizeable();
}
void xcpldapp::Dlsboard(){
	//do nothing right now
	sboard= (board_te)dlSBoard.GetIndex();
	UpdatePins();
}
void xcpldapp::Dlmconn(){
	mconnector = (connector_te)dlMConn.GetIndex();
	UpdatePins();
}
void xcpldapp::Dlmboard(){
	UpdatePins();
}
void xcpldapp::Apins(){
	//
}
String xcpldapp::GetCpuPort(String pin){
	#define DEF(conid, pins, pinx, pinm) if (conid==mconnector) if (pins == pin) return   #pinx " " pinm;
	#include "conpins.def"
	#undef DEF
	return "-";
}
void xcpldapp::UpdatePins(){
	aPins.Reset();
	aPins.AddColumn("Nr");
	aPins.AddColumn("Function");
	aPins.AddColumn("Pin");
	aPins.AddColumn("Description");
	aPins.AddColumn("Direction");
	aPins.AddColumn("Port");
	aPins.AllSorting();
	int nr=0;
	#define DEF(bid, fn, pin, desc, dir) if (sboard==bid){ aPins.Add(nr, fn, pin, desc, dir, GetCpuPort(pin)); nr++;};
	#include "board.def"
	#undef DEF
}
typedef enum{
#define DEF(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) p5,
#include "confpga.def"
#undef DEF
NumberOfConFpgaPin
}ConFpgaPin_te;
const char* g_FpgaPin[NumberOfConFpgaPin]={
	#define DEF(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) #p5,
	#include "confpga.def"
	#undef DEF
};

typedef enum{
#define DEF(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) p1,
#include "confpga.def"
#undef DEF
NumberOfConBoardPin
}ConBoardPin_te;
const char* g_BoardPin[NumberOfConBoardPin]={
	#define DEF(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) #p1,
	#include "confpga.def"
	#undef DEF
};

String GetPin(String bp)
{
	for(int i=0;i <NumberOfConBoardPin; i++){
		if (g_BoardPin[i] == bp) return g_FpgaPin[i];
	}
	return "";
}
String GetQsf(String bp){
	return "set_location_assignment "+ GetPin(bp)+" -to "+bp+"\n";
}
void xcpldapp::Bnverilog(){
//TODO: a lot...
	FileOut s("out.v");
	FileOut qsf("out.qsf");
	int lines=25;


	Bus data("data", tReg);
	Bus dir("dir", tReg);
	Bus lineA("lineA", tWire);
	Bus lineB("lineB", tWire);
	Bus CA("CA", tInOut);
	Bus CB("CB", tInOut);
	
	data.SetCount(lines);
	dir.SetCount(lines);
	lineA.SetCount(lines);
	lineB.SetCount(lines);
	CA.SetCount(lines);
	CB.SetCount(lines);
	
	dir[3].SetValue(1);
	
	s<<"//Generated by xcpldapp\n";
	s<<"module xmos_config\n";
	s<<"#( parameter WIDTH = ";
	s<<lines;
	s<<" )\n";
	s<<"( nRST, CLK, ";

	data.SetBusSize("lines");
	dir.SetBusSize("lines");
	lineA.SetBusSize("lines");
	lineB.SetBusSize("lines");
	CA.SetBusSize("lines");
	CB.SetBusSize("lines");

	s<<CA.VerilogID();
	s<<", ";
	s<<CB.VerilogID();
	s<<"\n)\n";
	
	s<<"localparam lines ="<<lines<<";\n\n";
	
	s<<"input nRST;\n";
	s<<"input CLK;\n";

	s<< CA.VerilogDecl();
	s<< CB.VerilogDecl();
	s<<"\n";
	
	s<< data.VerilogDecl();
	s<< lineA.VerilogDecl();
	s<< lineB.VerilogDecl();
	s<< dir.VerilogDecl();
	s<<"\n";
	
	for (int i=0; i<lines; i++){
		XPin xpA, xpB;
		xpA.setI(data[i]).setOE(dir[i]).setO(lineA[i]).setIO(CA[i]);
		xpB.setI(data[i]).setOE(dir[i]).setO(lineB[i]).setIO(CB[i]);
		s<< xpA.VerilogInstance();
		s<< xpB.VerilogInstance();
	}
	
	s<<"\nalways @(posedge CLK or negedge nRST)\nbegin\n";
	s<<"  if (~nRST)\n  begin\n";
	s<<"    dir =  ";
	s<< dir.GetValue();
	s<<";  // reset state (SETUP_initial)\n";
	s<<"    data=0;\n";
	s<<"  end\n";
	s<<"  else\n  begin\n";
	s<<"    for (integer i=rest; i<lines; i=i+1) data[i] = dir[i]?lineB[i]:lineA[i];\n";
	s<<"  end\n";
	s<<"end //always clk or nRST\n\n";
	s<<"endmodule;\n\n";
	s.Close();
	
	/*
	//todo: rename the pin bus
	qsf << GetQsf("CLK");
	qsf << GetQsf("nRST");
	for (int i=0; i<lines; i++){
		qsf << GetQsf(CA[i].VerilogID());
		qsf << GetQsf(CB[i].VerilogID());
	}*/
	for (int i=0; i<NumberOfConBoardPin; i++){
		qsf << GetQsf(g_BoardPin[i]);
	}
	qsf.Close();
	
}
GUI_APP_MAIN
{
	xcpldapp().Run();
}
