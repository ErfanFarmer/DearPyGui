#include "mvAppItem.h"
#include "mvApp.h"
#include "Core/mvInput.h"
#include "Core/mvDataStorage.h"
#include "mvPythonTranslator.h"
#include "Core/mvPythonExceptions.h"
#include "mvMarvel.h"

namespace Marvel{

	mvAppItem::mvAppItem(const std::string& name)
	{
		m_name = name;
		m_label = name;
	}

	void mvAppItem::checkConfigDict(PyObject* dict)
	{
		if (dict == nullptr)
			return;
		mvGlobalIntepreterLock gil;
		auto configKeys = ToStringVect(PyDict_Keys(dict));
		auto parserKeywords = (*mvApp::GetApp()->getParsers())[getParserCommand()].getKeywords();
		if (parserKeywords.empty())
		{
			ThrowPythonException("\"" + getName() + "\" could not find a parser that matched \"" + getParserCommand() + "\".");
			return;
		}
		for (auto key : configKeys)
		{
			int i = 0;
			while (i < parserKeywords.size() - 1)
			{
				if (key == parserKeywords[i])
				{
					break;
				}
				i++;
			}
			if (i == parserKeywords.size() - 1)
			{
				ThrowPythonException("\"" + key + "\" configuration does not exist in \"" + getName() + "\".");
			}
		}
	}

	void mvAppItem::setConfigDict(PyObject* dict)
	{
		if (dict == nullptr)
			return;
		mvGlobalIntepreterLock gil;
		if (PyObject* item = PyDict_GetItemString(dict, "name")) m_name = ToString(item);
		if (PyObject* item = PyDict_GetItemString(dict, "label")) setLabel(ToString(item));
		if (PyObject* item = PyDict_GetItemString(dict, "popup")) setPopup(ToString(item));
		if (PyObject* item = PyDict_GetItemString(dict, "tip")) setTip(ToString(item));
		if (PyObject* item = PyDict_GetItemString(dict, "width")) setWidth(ToInt(item));
		if (PyObject* item = PyDict_GetItemString(dict, "height")) setHeight(ToInt(item));
		if (PyObject* item = PyDict_GetItemString(dict, "show")) m_show =ToBool(item);
		if (PyObject* item = PyDict_GetItemString(dict, "data_source")) setDataSource(ToString(item));
	}

	void mvAppItem::getConfigDict(PyObject* dict)
	{
		if (dict == nullptr)
			return;
		mvGlobalIntepreterLock gil;
		PyDict_SetItemString(dict, "name", ToPyString(m_name));
		PyDict_SetItemString(dict, "label", ToPyString(m_label));
		PyDict_SetItemString(dict, "data_source", ToPyString(m_dataSource));
		PyDict_SetItemString(dict, "popup", ToPyString(m_popup));
		PyDict_SetItemString(dict, "tip", ToPyString(m_tip));
		PyDict_SetItemString(dict, "width", ToPyInt(m_width));
		PyDict_SetItemString(dict, "height", ToPyInt(m_height));
		PyDict_SetItemString(dict, "show", ToPyBool(m_show));
	}

	void mvAppItem::registerWindowFocusing()
	{
		if (ImGui::IsWindowFocused())
		{

			// update mouse
			ImVec2 mousePos = ImGui::GetMousePos();
			mvInput::setGlobalMousePosition(mousePos.x, mousePos.y);
			float x = mousePos.x - ImGui::GetWindowPos().x;
			float y = mousePos.y - ImGui::GetWindowPos().y;
			mvInput::setMousePosition(x, y);
			mvApp::GetApp()->setActiveWindow(m_name);

		}
	}

	void mvAppItem::setCallback(PyObject* callback) 
	{ 
		if (callback == Py_None)
		{
			m_callback = nullptr;
			return;
		}
		m_callback = callback; 
	}

	void mvAppItem::resetState()
	{
		m_hovered = false;
		m_active = false;
		m_focused = false;
		m_clicked = false;
		m_visible = false;
		m_edited = false;
		m_activated = false;
		m_deactivated = false;
		m_deactivatedAfterEdit = false;
		m_toggledOpen = false;

		for (mvAppItem* item : m_children)
			item->resetState();

	}

	bool  mvAppItem::moveChildUp(const std::string& name)
	{
		bool found = false;
		int index = 0;

		// check children
		for (size_t i = 0; i<m_children.size(); i++)
		{

			if (m_children[i]->getName() == name)
			{
				found = true;
				index = i;
				break;
			}

			if (m_children[i]->isContainer())
			{
				found = m_children[i]->moveChildUp(name);
				if (found)
					return true;
			}

		}

		if (found)
		{
			if (index > 0)
			{
				auto upperitem = m_children[index - 1];
				auto loweritem = m_children[index];

				m_children[index] = upperitem;
				m_children[index-1] = loweritem;
			}

			return true;
		}

		return false;

	}

	bool  mvAppItem::moveChildDown(const std::string& name)
	{
		bool found = false;
		int index = 0;

		// check children
		for (size_t i = 0; i < m_children.size(); i++)
		{

			if (m_children[i]->getName() == name)
			{
				found = true;
				index = i;
				break;
			}

			if (m_children[i]->isContainer())
			{
				found = m_children[i]->moveChildDown(name);
				if (found)
					return true;
			}

		}

		if (found)
		{
			if (index < m_children.size()-1)
			{
				auto upperitem = m_children[index];
				auto loweritem = m_children[index+1];

				m_children[index] = loweritem;
				m_children[index + 1] = upperitem;
			}

			return true;
		}

		return false;
	}

	void mvAppItem::updateDataSource(const std::string& name)
	{

		updateData(name);

		for (auto child : m_children)
			child->updateDataSource(name);
	}

	bool mvAppItem::addRuntimeChild(const std::string& parent, const std::string& before, mvAppItem* item)
	{
		if (before.empty() && parent.empty())
			return false;

		//this is the container, add item to beginning.
		if (before.empty())
		{
			if (parent == m_name)
			{
				m_children.push_back(item);
				item->setParent(this);
				return true;
			}

			else
			{
				// check children
				for (mvAppItem* child : m_children)
				{
					if (child->isContainer())
					{
						// parent found
						if (child->addRuntimeChild(parent, before, item))
							return true;
					}
				}
			}
		}

		else
		{
			bool beforeFound = false;

			// check children
			for (mvAppItem* child : m_children)
			{

				if (child->getName() == before)
				{
					beforeFound = true;
					break;
				}

			}

			// after item is in this container
			if (beforeFound)
			{
				item->setParent(this);

				std::vector<mvAppItem*> oldchildren = m_children;
				m_children.clear();

				for (auto child : oldchildren)
				{
					if (child->getName() == before)
						m_children.push_back(item);
					m_children.push_back(child);

				}

				return true;
			}
		}

		// check children
		for (mvAppItem* child : m_children)
		{
			if (child->isContainer())
			{
				// parent found
				if (child->addRuntimeChild(parent, before, item))
					return true;
			}
		}

		return false;
	}

	bool mvAppItem::addChildAfter(const std::string& prev, mvAppItem* item)
	{
		if (prev.empty())
			return false;


		bool prevFound = false;

		// check children
		for (mvAppItem* child : m_children)
		{

			if (child->getName() == prev)
			{
				item->setParent(this);
				prevFound = true;
				break;
			}

		}

		// prev item is in this container
		if (prevFound)
		{
			//item->setParent(this);

			std::vector<mvAppItem*> oldchildren = m_children;
			m_children.clear();

			for (auto child : oldchildren)
			{
				m_children.push_back(child);
				if (child->getName() == prev)
					m_children.push_back(item);
			}

			return true;
		}
		

		// check children
		for (mvAppItem* child : m_children)
		{
			if (child->isContainer())
			{
				// parent found
				if (child->addChildAfter(prev, item))
					return true;
			}
		}

		return false;
	}

	bool mvAppItem::deleteChild(const std::string& name)
	{

		bool childfound = false;
		bool itemDeleted = false;

		for (mvAppItem* item : m_children)
		{
			if (item->getName() == name)
			{
				childfound = true;
				break;
			}

			if (item->isContainer())
			{
				itemDeleted = item->deleteChild(name);
				if (itemDeleted)
					break;
			}
		}

		if (childfound)
		{
			std::vector<mvAppItem*> oldchildren = m_children;

			m_children.clear();

			for (auto& item : oldchildren)
			{
				if (item->getName() == name)
				{
					delete item;
					item = nullptr;
					itemDeleted = true;
					continue;
				}

				m_children.push_back(item);
			}
		}

		return itemDeleted;
	}

	void mvAppItem::deleteChildren()
	{
		for (auto& child : m_children)
		{
			delete child;
			child = nullptr;
		}

		m_children.clear();
	}

	void mvAppItem::showAll()
	{
		m_show = true;
		for (auto child : m_children)
			child->showAll();
	}

	void mvAppItem::hideAll()
	{
		if (getType() != mvAppItemType::Tooltip)
			m_show = false;

		for (auto child : m_children)
			child->hideAll();
	}

	void mvAppItem::setParent(mvAppItem* parent)
	{
		m_parent = parent;
	}

	void mvAppItem::addColorStyle(ImGuiCol item, mvColor color)
	{
		m_colors.push_back({ item,  color });
	}

	void mvAppItem::pushColorStyles()
	{
		for (auto& item : m_colors)
			ImGui::PushStyleColor(item.idx, item.color);
	}

	void mvAppItem::popColorStyles()
	{
		if(!m_colors.empty())
			ImGui::PopStyleColor(m_colors.size());
	}

	void mvAppItem::clearColors()
	{
		m_colors.clear();
	}

	mvAppItem* mvAppItem::getChild(const std::string& name)
	{
		for (mvAppItem* item : m_children)
		{
			if (item->getName() == name)
				return item;

			if (item->isContainer())
			{
				auto child = item->getChild(name);
				if (child)
					return child;
			}
		}

		return nullptr;
	}

	void mvAppItem::addChild(mvAppItem* child)
	{
		m_container = true;
		m_children.push_back(child);
	}

	mvAppItem::~mvAppItem()
	{
		deleteChildren();

		if (m_callback)
			Py_DECREF(m_callback);
		if (m_callbackData)
			Py_DECREF(m_callbackData);
	}

}