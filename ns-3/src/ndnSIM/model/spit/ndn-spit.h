/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Jin Pengfei <jinpengfei@cstnet.cn>
 */

#ifndef _NDN_SPIT_H_
#define	_NDN_SPIT_H_

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"

#include "ndn-spit-entry.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn
 * @defgroup ndn-spit SPIT
 */

/**
 * @ingroup ndn-spit
 * @brief Namespace for SPIT operations
 */
namespace spit {
}

class L3Protocol;
class Face;
class Data;
class Interest;

typedef Interest InterestHeader;
typedef Data DataHeader;

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

/**
 * @ingroup ndn-spit
 * @brief Class implementing Pending Interests Table
 */
class SPit : public Object
{
public:
  /**
   * \brief Interface ID
   *
   * \return interface ID
   */
  static TypeId GetTypeId ();

  /**
   * \brief SPIT constructor
   */
  SPit ();

  /**
   * \brief Destructor
   */
  virtual ~SPit ();

  /**
   * \brief Find corresponding SPIT entry for the given content name
   *
   * Not that this call should be repeated enough times until it return 0.
   * This way all records with shorter or equal prefix as in content object will be found
   * and satisfied.
   *
   * \param prefix Prefix for which to lookup the entry
   * \returns smart pointer to SPIT entry. If record not found,
   *          returns 0
   */
  virtual Ptr<spit::Entry>
  Lookup (const Data &header) = 0;

  /**
   * \brief Find a SPIT entry for the given content interest
   * \param header parsed interest header
   * \returns iterator to SPit entry. If record not found,
   *          return end() iterator
   */
  virtual Ptr<spit::Entry>
  Lookup (const Interest &header) = 0;

  /**
   * @brief Get SPIT entry for the prefix (exact match)
   *
   * @param prefix Name for SPIT entry
   * @returns If entry is found, a valid iterator (Ptr<spit::Entry>) will be returned. Otherwise End () (==0)
   */
  virtual Ptr<spit::Entry>
  Find (const Name &prefix) = 0;

  /**
   * @brief Creates a SPIT entry for the given interest
   * @param header parsed interest header
   * @returns iterator to SPit entry. If record could not be created (e.g., limit reached),
   *          return end() iterator
   *
   * Note. This call assumes that the entry does not exist (i.e., there was a Lookup call before)
   */
  virtual Ptr<spit::Entry>
  Create (Ptr<const Interest> header) = 0;

  /**
   * @brief Mark SPIT entry deleted
   * @param entry SPIT entry
   *
   * Effectively, this method removes all incoming/outgoing faces and set
   * lifetime +m_SPitEntryDefaultLifetime from Now ()
   */
  virtual void
  MarkErased (Ptr<spit::Entry> entry) = 0;

  /**
   * @brief Print out SPIT contents for debugging purposes
   *
   * Note that there is no definite order in which entries are printed out
   */
  virtual void
  Print (std::ostream &os) const = 0;

  /**
   * @brief Get number of entries in SPIT
   */
  virtual uint32_t
  GetSize () const = 0;

  /**
   * @brief Return first element of FIB (no order guaranteed)
   */
  virtual Ptr<spit::Entry>
  Begin () = 0;

  /**
   * @brief Return item next after last (no order guaranteed)
   */
  virtual Ptr<spit::Entry>
  End () = 0;

  /**
   * @brief Advance the iterator
   */
  virtual Ptr<spit::Entry>
  Next (Ptr<spit::Entry>) = 0;

  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Static call to cheat python bindings
   */
  static inline Ptr<SPit>
  GetSPit (Ptr<Object> node);

  /**
   * @brief Get maximum SPIT entry lifetime
   */
  inline const Time&
  GetMaxSPitEntryLifetime () const;

  /**
   * @brief Set maximum SPIT entry lifetime
   */
  inline void
  SetMaxSPitEntryLifetime (const Time &maxLifetime);

protected:
  // configuration variables. Check implementation of GetTypeId for more details
  Time m_SPitEntryPruningTimout;

  Time m_maxSPitEntryLifetime;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline std::ostream&
operator<< (std::ostream& os, const SPit &spit)
{
  spit.Print (os);
  return os;
}

inline Ptr<SPit>
SPit::GetSPit (Ptr<Object> node)
{
  return node->GetObject<SPit> ();
}

inline const Time&
SPit::GetMaxSPitEntryLifetime () const
{
  return m_maxSPitEntryLifetime;
}

inline void
SPit::SetMaxSPitEntryLifetime (const Time &maxLifetime)
{
  m_maxSPitEntryLifetime = maxLifetime;
}


} // namespace ndn
} // namespace ns3

#endif	/* NDN_SPIT_H */
