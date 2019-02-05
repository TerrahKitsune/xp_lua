#include "CNWSVarTable.h"
#include "NWN2_API.h"

void ClearVariables(CNWSVarTable * vars) {

	if (!vars || !vars->vartable || vars->vartable_alloc <= 0) {
		return;
	}

	for (size_t i = 0; i < vars->vartable_len; i++)
	{
		CScriptVariable * var = &vars->vartable[i];

		if (var->Name.text) {
			NWN2_Free(var->Name.text);
		}

		CExoString *str;

		switch (var->Type)
		{
		case 3:
			str = (CExoString*)var->Data;
			if (str->text) {
				NWN2_Free(str->text);
				str->text = NULL;
				str->len = 0;
			}
			var->Data = 0;
			break;
		case 5:
			var->Data = 0;
			break;
		default:
			break;
		}
	}

	vars->vartable_alloc = 0;
	vars->vartable_len = 0;
	NWN2_Free(vars->vartable);
	vars->vartable = NULL;
}