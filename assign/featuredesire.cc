/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * featuredesire.cc - implementation of the objects from featuredesire.h
 */

#include "featuredesire.h"
#include <iostream>
using namespace std;

/*********************************************************************
 * tb_featuredesire
 *********************************************************************/
tb_featuredesire::name_featuredesire_map
    tb_featuredesire::featuredesires_by_name;

/*
 * Constructor
 */
tb_featuredesire::tb_featuredesire(fstring _my_name) : my_name(_my_name),
				    global(false), local(false),
				    l_additive(false), g_one_is_okay(false),
				    g_more_than_one(false),
				    in_use_globally(0), desire_policy(),
				    feature_policy(), desire_users(0),
				    desire_total_weight(0.0f) { 
    static int highest_id = 0;
		
    // Pick a unique numeric identifier for this feature/desire
    id = highest_id++;

    // Find out which flags should be set for this feature/desire from the name
    switch (my_name[0]) {
	case '?':
	    // A local feature - the second character tell us what kind.
	    // Currently only additive are supported
	    local = true;
	    switch(my_name[1]) {
		case '+':
		    l_additive = true;
		    break;
		default:
		    cerr << "*** Invalid local feature type in feature " <<
			my_name << endl;
		    exit(EXIT_FATAL);
	    }
	    break;
	case '*':
	    // A global feature - the second character tells us what kind.
	    global = true;
	    switch(my_name[1]) {
		case '&':
		    g_one_is_okay = true;
		    break;
		case '!':
		    g_more_than_one = true;
		    break;
		default:
		    cerr << "*** Invalid global feature type in feature " <<
			my_name << endl;
		    exit(EXIT_FATAL);
	    }
	    break;
	default:
	    // Just a regular feature
	    break;
    }

    // Place this into the map for finding featuredesire objects by
    // name
    assert(featuredesires_by_name.find(my_name)
	    == featuredesires_by_name.end());
    featuredesires_by_name[my_name] = this;
}

/*
 * Operators
 */
ostream &operator<<(ostream &o, const tb_featuredesire &fd) {
    // Perhaps this should print more information like the flags and/or global
    // use count
    o << fd.my_name;
}


/*
 * Static functions
 */
tb_featuredesire *tb_featuredesire::get_featuredesire_obj(const fstring name) {
    name_featuredesire_map::iterator it =
	featuredesires_by_name.find(name);
    if (it == featuredesires_by_name.end()) {
	return new tb_featuredesire(name);
    } else {
	return it->second;
    }
}

/*
 * Functions for maintaining state for global FDs
 */
void tb_featuredesire::add_global_user(int howmany) {
    assert(global);
    in_use_globally += howmany;
}	

void tb_featuredesire::remove_global_user(int howmany) {
    assert(global);
    in_use_globally -= howmany;
    assert(in_use_globally >= 0);
}

/*
 * Functions for manipulating policies
 */
void tb_featuredesire::disallow_desire() {
    desire_policy.disallow();
}
void tb_featuredesire::allow_desire() {
    desire_policy.allow();
}
void tb_featuredesire::limit_desire(double limit) {
    desire_policy.limit_use(limit);
}
void tb_featuredesire::unlimit_desire() {
    desire_policy.unlimit_use();
}

/*
 * Bookkeeping functions
 */
void tb_featuredesire::add_desire_user(double weight) {
    desire_users++;
    desire_total_weight += weight;
}
int tb_featuredesire::get_desire_users() {
    return desire_users;
}
double tb_featuredesire::get_desire_total_weight() {
    return desire_total_weight;
}

/*
 * Check desire violations - true means everything was okay, false otherwise
 */
bool tb_featuredesire::check_desire_policies() {
    int errors = 0;
    name_featuredesire_map::const_iterator it = featuredesires_by_name.begin();
    while (it != featuredesires_by_name.end()) {
	tb_featuredesire *fd = it->second;
	if (!fd->desire_policy.is_allowable() && fd->desire_users) {
	    cout << "  *** Policy violation: " << endl
		<< "      Feature " << it->first
		<< " requested, but prohibited by policy" << endl;
	    errors++;
	} else {
	    if (fd->desire_policy.is_limited() &&
		(fd->desire_total_weight > fd->desire_policy.get_limit())) {
		cout << "  *** Policy violation: " << endl
		    << "    Feature " << it->first
		    << " requested with weight " << fd->desire_total_weight
		    << " but limted to " << fd->desire_policy.get_limit()
		    << " by policy" << endl;
		errors++;
	    }
	}
	it++;
    }
    
    if (errors) {
	return false;
    } else {
	return true;
    }
}



/*********************************************************************
 * tb_node_featuredesire
 *********************************************************************/

/*
 * Constructor
 */
tb_node_featuredesire::tb_node_featuredesire(fstring _name, double _weight) :
	weight(_weight), violateable(false), used_local_capacity(0.0f) {
    // We'll want to change in the in the future to seperate out the notions of
    // score and violations
    if (weight >= FD_VIOLATION_WEIGHT) {
	violateable = true;
    }
    featuredesire_obj = tb_featuredesire::get_featuredesire_obj(_name);
    assert(featuredesire_obj != NULL);
}

/*
 * Add a global user of this feature, and return the score and violations
 * induced by this
 */
score_and_violations tb_node_featuredesire::add_global_user() const {
    featuredesire_obj->add_global_user();

    double score = 0.0f;
    int violations = 0;

    // Handle the scoring and violations from this change
    if (featuredesire_obj->is_g_one() &&
	    (featuredesire_obj->global_use_count() >= 2)) {
	// Have to penalize this one, it's not the first
	score += weight;
	if (violateable) {
	    violations += 1;
	}
    } else if (featuredesire_obj->is_g_more() &&
	    (featuredesire_obj->global_use_count() == 1)) {
	// Only penalize the first one
	score += weight;
	if (violateable) {
	    violations += 1;
	}
    }
    return score_and_violations(score,violations);
}


/*
 * Remove a global user of this feature, and return the score and violations
 * induced by this
 */
score_and_violations tb_node_featuredesire::remove_global_user() const {
    featuredesire_obj->remove_global_user();

    double score = 0.0f;
    int violations = 0;

    // Handle the scoring and violations from this change
    if (featuredesire_obj->is_g_one() &&
	    (featuredesire_obj->global_use_count() >= 1)) {
	// We're not removing the final one, so we count it
	score += weight;
	if (violateable) {
	    violations += 1;
	}
    } else if (featuredesire_obj->is_g_more() &&
	    (featuredesire_obj->global_use_count() == 0)) {
	// Only count it if we just removed the final one
	score += weight;
	if (violateable) {
	    violations += 1;
	}
    }
    return score_and_violations(score,violations);
}

/*
 * Add a local user of a feature
 * XXX - Should we add more violations if we hit multiples of the capacity?
 * XXX - These are always assumed to be violatable
 */
score_and_violations tb_node_featuredesire::add_local(double amount) {
    double oldvalue = used_local_capacity;
    used_local_capacity += amount;
    if ((oldvalue <= weight) && (used_local_capacity > weight)) {
	// This one pushed us over the edge, violation!
	return score_and_violations(SCORE_OVERUSED_LOCAL_FEATURE,1);
    } else {
	// We're good, doesn't cost anything;
	return score_and_violations(0.0f,0);
    }
}

/*
 * Subtract a local user of a feature
 */
score_and_violations tb_node_featuredesire::subtract_local(double amount) {
    double oldvalue = used_local_capacity;
    used_local_capacity -= amount;
    if ((oldvalue > weight) && (used_local_capacity <= weight)) {
	// Back down to below capacity, remove a violation
	return score_and_violations(SCORE_OVERUSED_LOCAL_FEATURE,1);
    } else {
	// We're good, doesn't cost anything;
	return score_and_violations(0.0f,0);
    }

}

/*********************************************************************
 * tb_featuredesire_set_iterator
 *********************************************************************/

tb_featuredesire_set_iterator::tb_featuredesire_set_iterator(
	node_fd_set::iterator _begin1, node_fd_set::iterator _end1,
	node_fd_set::iterator _begin2, node_fd_set::iterator _end2) :
	    it1(_begin1), end1(_end1), it2(_begin2), end2(_end2) {
    /*
     * Figure out what the next element of the set is
     */
    // First check to see if we've hit the end of both lists
    if ((it1 == end1) && (it2 == end2)) {
	current = end1;
    } else if ((it1 != end1) && ((it2 == end2) || (*it1 < *it2))) {
	// If one has hit the end of the list, go with the other - otherwise,
	// go with the smaller of the two.
	current_membership = FIRST_ONLY;
	current = it1;
    } else if ((it1 == end1) || (*it2 < *it1)) {
	current_membership = SECOND_ONLY;
	current = it2;
    } else {
	// If neither is smaller, they must be equal
	current = it1;
	current_membership = BOTH;
    }
}

bool tb_featuredesire_set_iterator::done() const {
    return((it1 == end1) && (it2 == end2));
}

void tb_featuredesire_set_iterator::operator++(int) {
    /*
     * Advance the iterator(s)
     */
    // Make sure they don't try to go off the end of the list
    assert(!done());
    // If one iterator has gone off the end of its list, advance the other one
    // - otherwise, go with the smaller one. Or, if they are equal,  increment
    // both.
    if ((it1 != end1) && ((it2 == end2) ||  (*it1 < *it2))) {
	it1++;
    } else if ((it1 == end1) || (*it2 < *it1)) {
	it2++;
    } else {
	// If neither was smaller, they must be equal - advance both
	it1++;
	it2++;
    }

    /*
     * Figure out what the next element of the set is
     */
    // First check to see if we've hit the end of both lists
    if ((it1 == end1) && (it2 == end2)) {
	current = end1;
    } else if ((it2 == end2) || ((it1 != end1) && (*it1 < *it2))) {
	// If one has hit the end of the list, go with the other - otherwise,
	// go with the smaller of the two.
	current_membership = FIRST_ONLY;
	current = it1;
    } else if ((it1 == end1) || (*it2 < *it1)) {
	current_membership = SECOND_ONLY;
	current = it2;
    } else {
	// If neither is smaller, they must be equal
	current = it1;
	current_membership = BOTH;
    }
}

bool tb_featuredesire_set_iterator::both_equiv() const {
    assert(current_membership == BOTH);
    return (it1->equivalent(*it2));
}

bool tb_featuredesire_set_iterator::either_violateable() const {
    if (current_membership == BOTH) {
	return (it1->is_violateable() || it2->is_violateable());
    } else {
	return current->is_violateable();
    }
}
