#include "PigModel.h"

PigModel::PigModel( float g /*= 0*/ )
:	super(6, g)
{
	head.texOffs(16, 16).addBox(-2, 0, -9, 4, 3, 1, g);
	yHeadOffs = 4;
}
