/*MIT License

Copyright (c) 2018 Florian GERARD

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Except as contained in this notice, the name of Florian GERARD shall not be used 
in advertising or otherwise to promote the sale, use or other dealings in this 
Software without prior written authorization from Florian GERARD

*/


#pragma once

#include <cstdint>


namespace framework
{
	template<typename UnderLyingType, typename List>
		class DualLinkNode
		{
			
			template<typename T,typename L> friend class DualLinkedList;
			
		private:
			DualLinkNode<UnderLyingType,List>* m_previous;
			DualLinkNode<UnderLyingType,List>* m_next;
			
			//place node before this
			void prepend(DualLinkNode<UnderLyingType, List>* node)
			{
				if (node != nullptr)
				{
					node->m_previous = m_previous;
					node->m_next = this;
					
					m_previous = node;
					
					if (node->m_previous != nullptr)
						node->m_previous->m_next = node;
				}
			}
			
			//place node after this
			void append(DualLinkNode<UnderLyingType, List>* node)
			{
				if (node != nullptr)
				{
					node->m_previous = this;
					node->m_next = m_next;
					
					m_next = node;
					
					if (node->m_next != nullptr)
						node->m_next->m_previous = node;
				}
			}
			
		public:
			
			constexpr DualLinkNode()
			{
				m_previous = nullptr;
				m_next = nullptr;
			}
			
			
			static void next(UnderLyingType* node, UnderLyingType* next)
			{
				if (node == nullptr)
					return;
				static_cast<DualLinkNode<UnderLyingType, List>*>(node)->m_next = static_cast<DualLinkNode<UnderLyingType, List>>(next);
			}
			
			static void previous(UnderLyingType node,UnderLyingType* previous)
			{
				if (node == nullptr)
					return;
				static_cast<DualLinkNode<UnderLyingType, List>*>(node)->m_previous = static_cast<DualLinkNode<UnderLyingType, List>*>(previous);
			}
			
			static UnderLyingType* next(UnderLyingType* node)
			{
				if (node == nullptr)
					return nullptr;
				return static_cast<UnderLyingType*>(static_cast<DualLinkNode<UnderLyingType, List>*>(node)->m_next);
			}
			
			static UnderLyingType* previous(UnderLyingType* node)
			{
				if (node == nullptr)
					return nullptr;
				return static_cast<UnderLyingType*>(static_cast<DualLinkNode<UnderLyingType, List>*>(node)->m_previous);
			}
		};
	
	
	
	template<typename UnderLyingType, typename List>
		class DualLinkedList
		{
			
		private:
			DualLinkNode<UnderLyingType,List>* m_first;
			uint32_t m_count;
			
		public:
			
			/*Given comparator function,  
			 *if return value is >0 compared inferior to base
			 *if return value is 0 compared and base are equal
			 *if return value is <0 compared superior to base*/
			typedef int8_t(*Comparator)(UnderLyingType* base, UnderLyingType* compared);
			
			bool insert(UnderLyingType* node, Comparator comparator)
			{
				if (node == nullptr)
					return false;
				DualLinkNode<UnderLyingType, List>* newNode = static_cast<DualLinkNode<UnderLyingType, List>*>(node);
				newNode->m_next = nullptr;
				newNode->m_previous = nullptr;
				if (m_count == 0)
				{
					m_first = newNode;
					m_count = 1;
					return true;
				}
				else
				{
					uint32_t i = 0;
					DualLinkNode<UnderLyingType,List>* iterator = m_first;
					int8_t result = 0;
					
					/*Go throught the list until last element*/
					while (i < m_count-1)
					{
						result = comparator(static_cast<UnderLyingType*>(iterator), static_cast<UnderLyingType*>(newNode));
						if (result > 0)
						{
							iterator->prepend(newNode);
							m_count++;
							if (iterator == m_first)
								m_first = newNode;
							return true;
						}
						else
						{
							iterator = iterator->m_next;
							i++;
						}
					}
					//this is the last element
					result = comparator(static_cast<UnderLyingType*>(iterator), static_cast<UnderLyingType*>(newNode));
					if (result < 1)
						iterator->append(newNode);
					else
					{	
						iterator->prepend(newNode);
						if (iterator == m_first)
							m_first = newNode;
					}
					m_count++;
					return true;
				}
			}
			
			
			bool remove(UnderLyingType* node)
			{
				DualLinkNode<UnderLyingType, List>* newNode = static_cast<DualLinkNode<UnderLyingType,List>*>(node);
				DualLinkNode<UnderLyingType,List>* iteratorPtr = m_first;
				uint32_t iteratorCounter = 0;
				while (iteratorCounter < m_count)
				{
					if (iteratorPtr == newNode)
					{
						if (iteratorPtr->m_next != nullptr)
							iteratorPtr->m_next->m_previous = iteratorPtr->m_previous;
						
						if (iteratorPtr->m_previous != nullptr)
							iteratorPtr->m_previous->m_next = iteratorPtr->m_next;
						
						iteratorPtr->m_next = nullptr;
						iteratorPtr->m_previous = nullptr;
						m_count--;
						return true;
					}
					iteratorPtr = iteratorPtr->m_next;
					iteratorCounter++; 
				}
				return false;
			}
			
			
			bool isEmpty()
			{
				if (m_count == 0)
					return true;
				else
					return false;
			}
			
			bool contain(UnderLyingType* node)
			{
				if (node == nullptr)
					return false;
				DualLinkNode<UnderLyingType, List>* toFindNode = static_cast<DualLinkNode<UnderLyingType, List>*>(node);
				DualLinkNode<UnderLyingType, List>* iteratorPtr = m_first;
				uint32_t iteratorCounter = 0;
				while (iteratorCounter < m_count)
				{
					if (iteratorPtr == toFindNode)
						return true;
					iteratorPtr = iteratorPtr->m_next;
					iteratorCounter++; 
				}
				return false;
			}
			
			UnderLyingType* peekFirst()
			{
				return static_cast<UnderLyingType*>(m_first);
			}
			
			UnderLyingType* getFirst()
			{
				if (m_count == 0)
					return nullptr;
				else
				{
					DualLinkNode<UnderLyingType,List>* ptr;
					ptr = m_first;
					m_count--;
					m_first = m_first->m_next;
					if(m_first != nullptr)
						m_first->m_previous = nullptr;
					
					ptr->m_next = nullptr;
					ptr->m_previous = nullptr;
					return static_cast<UnderLyingType*>(ptr);
				}
			}
			
			uint32_t count()
			{
				return m_count;
			}
		};
}
