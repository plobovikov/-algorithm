#pragma once
#include <memory>

template<class ValueType, template<class>class CombinePolicy>
struct three_node {
	std::shared_ptr<three_node> m_pRigth;
	std::shared_ptr<three_node> m_pLeft;
	std::shared_ptr<three_node> m_pParent;

	ValueType m_Value;

	three_node(const std::shared_ptr<three_node>& rRigth, const std::shared_ptr<three_node>& rLeft) :
		m_pRigth(rRigth), m_pLeft(rLeft), m_pParent(nullptr)
	{
		m_Value = CombinePolicy<ValueType>::combine_values(m_pRigth->m_Value, m_pLeft->m_Value);
	}
	three_node(const ValueType& value) : m_pRigth(nullptr), m_pLeft(nullptr), m_pParent(nullptr), m_Value(value) {

	}

	void set_parent(const std::shared_ptr<three_node>& pParent)
	{
		m_pParent = pParent;
	}
	bool is_root() const { return !m_pParent; }
	bool is_leaf() const { return !m_pLeft && !m_pRigth; }

};

template<typename ValueType>
struct adder_union_policy;

template<>
struct adder_union_policy<std::pair<char, int> > {
	typedef std::pair<char, int> value_type;
	static value_type combine_values(const value_type& rRigth, const value_type& rLeft) {
		return std::make_pair(char(), rRigth.second + rLeft.second);
	}
};