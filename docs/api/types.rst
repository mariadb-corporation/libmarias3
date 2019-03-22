Structs
=======

.. c:type:: ms3_st

   An internal struct which contains authentication information

.. c:type:: ms3_list_st

   A linked-list struct which contains a list of files/keys and information about them

   .. c:member:: char *key

      The key/filename for the object

   .. c:member:: size_t length

      The data size for the object

   .. c:member:: time_t created

      The created / updated timestamp for the object

   .. c:member:: struct ms3_list_st *next

      A pointer to the next struct in the list

.. c:type:: ms3_status_st

   An struct which contains the status of an object

   .. c:member:: size_t length

      The data size for the object

   .. c:member:: time_t created

      The created / updated timestamp for the object

Built-In Types
==============

.. c:type:: NULL

   A null pointer as defined in the standard header ``string.h``.

.. c:type:: uint8_t

   An unsigned single byte character as defined in the standard header ``stdint.h``

.. c:type:: bool

   A boolean type as defined in the standard header ``stdbool.h``

