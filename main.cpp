#include "Bim.h"

Bim::Bim()
{
	CtrlLayout(*this, "Bim");
	Sizeable().MinimizeBox().MaximizeBox();
}

GUI_APP_MAIN
{
	Bim().Run();
}
