#if 0

#include "UploadPhotoScreen.h"
#include "../renderer/TileRenderer.h"
#include "../player/LocalPlayer.h"
#include "../../world/entity/player/Inventory.h"

UploadPhotoScreen::UploadPhotoScreen()
	:
selectedItem(0)
{
}

void UploadPhotoScreen::init()
{
	int currentSelection = minecraft->player->inventory->getSelectedItemId();
	for (int i = 0; i < Inventory::INVENTORY_SIZE; i++)
	{
		if (currentSelection == minecraft->player->inventory->getSelectionSlotItemId(i + Inventory::SELECTION_SIZE))
		{
			selectedItem = i;
			break;
		}
	}

}

void UploadPhotoScreen::renderSlots()
{
	glColor4f2(1, 1, 1, 1);

	blitOffset = -90;

	minecraft->textures->loadAndBindTexture("gui/gui.png");
	for (int r = 0; r < Inventory::INVENTORY_ROWS; r++)
	{
		blit(width / 2 - 182 / 2, height - 22 * 3 - 22 * r, 0, 0, 182, 22);
	}
	if (selectedItem >= 0)
	{
		int x = width / 2 - 182 / 2 - 1 + (selectedItem % Inventory::SELECTION_SIZE) * 20;
		int y = height - 22 * 3 - 1 - (selectedItem / Inventory::SELECTION_SIZE) * 22;
		blit(x, y, 0, 22, 24, 22);
	}

	for (int r = 0; r < Inventory::INVENTORY_ROWS; r++)
	{
		for (int i = 0; i < 9; i++) {
			int x = width / 2 - 9 * 10 + i * 20 + 2;
			int y = height - 16 - 3 - 22 * 2 - 22 * r;
			renderSlot(r * 9 + i + Inventory::SELECTION_SIZE, x, y, 0);
		}
	}

}

void UploadPhotoScreen::renderSlot(int slot, int x, int y, float a)
{
	int itemId = minecraft->player->inventory->getSelectionSlotItemId(slot);
	if (itemId < 0) return;

	const bool fancy = false;

	if (fancy && itemId < 256 && TileRenderer::canRender(Tile::tiles[itemId]->getRenderShape())) {

	} else {
		if (itemId < 256) {
			Tile* tile = Tile::tiles[itemId];
			if (tile == NULL) return;

			minecraft->textures->loadAndBindTexture("terrain.png");

			int textureId = tile->getTexture(2, 0);
			blit(x, y, textureId % 16 * 16, textureId / 16 * 16, 16, 16);
		}
	}

}

void UploadPhotoScreen::keyPressed(int eventKey)
{
	int selX = selectedItem % Inventory::SELECTION_SIZE;
	int selY = selectedItem / Inventory::SELECTION_SIZE;

	Options& o = minecraft->options;
	if (eventKey == o.keyLeft.key && selX > 0)
	{
		selectedItem -= 1;
	}
	else if (eventKey == o.keyRight.key && selX < (Inventory::SELECTION_SIZE - 1))
	{
		selectedItem += 1;
	}
	else if (eventKey == o.keyDown.key && selY > 0)
	{
		selectedItem -= Inventory::SELECTION_SIZE;
	}
	else if (eventKey == o.keyUp.key && selY < (Inventory::INVENTORY_ROWS - 1))
	{
		selectedItem += Inventory::SELECTION_SIZE;
	}

	if (eventKey == o.keyMenuOk.key)
	{
		selectSlotAndClose();
	}
}

int UploadPhotoScreen::getSelectedSlot(int x, int y)
{
	int left = 3 + width / 2 - Inventory::SELECTION_SIZE * 10;
	int top = height - 16 - 3 - 22 * 2 - 22 * Inventory::INVENTORY_ROWS;

	if (x >= left && y >= top)
	{
		int xSlot = (x - left) / 20;
		if (xSlot < Inventory::SELECTION_SIZE)
		{
			// rows are rendered upsidedown
			return xSlot + Inventory::INVENTORY_SIZE - ((y - top) / 22) * Inventory::SELECTION_SIZE;
		}
	}
	return -1;
}

void UploadPhotoScreen::mouseClicked(int x, int y, int buttonNum) {
	if (buttonNum == MouseAction::ACTION_LEFT) {

		int slot = getSelectedSlot(x, y);
		if (slot >= 0 && slot < Inventory::INVENTORY_SIZE)
		{
			selectedItem = slot;
			//minecraft->soundEngine->playUI("random.click", 1, 1);
		}
	}
}

void UploadPhotoScreen::mouseReleased(int x, int y, int buttonNum)
{
	if (buttonNum == MouseAction::ACTION_LEFT) {

		int slot = getSelectedSlot(x, y);
		if (slot >= 0 && slot < Inventory::INVENTORY_SIZE && slot == selectedItem)
		{
			selectSlotAndClose();
		}
	}
}

void UploadPhotoScreen::selectSlotAndClose()
{
	Inventory* inventory = minecraft->player->inventory;

	int itemId = inventory->getSelectionSlotItemId(selectedItem + Inventory::SELECTION_SIZE);
	int i = 0;

	for (; i < Inventory::SELECTION_SIZE - 2; i++)
	{
		if (itemId == inventory->getSelectionSlotItemId(i))
		{
			break;
		}
	}

	// update selection list
	for (; i >= 1; i--)
	{
		inventory->setSelectionSlotItemId(i, inventory->getSelectionSlotItemId(i - 1));
	}
	inventory->setSelectionSlotItemId(0, itemId);
	inventory->selectSlot(0);

	minecraft->soundEngine->playUI("random.click", 1, 1);
	minecraft->setScreen(NULL);
}

#endif
