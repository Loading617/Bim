#ifndef _Bim_Bim_h
#define _Bim_Bim_h

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

#define LAYOUTFILE <Bim/Bim.lay>
#include <CtrlCore/lay.h>

class Bim : public WithBimLayout<TopWindow> {
public:
	Bim();
};

#endif
