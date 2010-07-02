(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 *)

type h = int Heap.heap;;

let h1 = Heap.make_heap 0;;
(* Should be: int Heap.heap = <abstr> *)

Heap.insert h1 5 1;;
(* Should be: fun from int to unit *)

Heap.min h1;;
(* Should be: (5,1) *)

Heap.insert h1 10 2;;
(* Should be: fun from int to unit *)

Heap.min h1;;
(* Should be: (5,1) *)

Heap.insert h1 2 3;;
(* Should be: fun from int to unit *)

Heap.min h1;;
(* Should be: (2,3) *)

Heap.extract_min h1;;
Heap.min h1;;
(* Should be: (5,1) *)

Heap.extract_min h1;;
Heap.min h1;;
(* Should be: (10,2) *)

Heap.extract_min h1;;
Heap.min h1;;
(* Should raise Heap.EmptyHeap *)
Heap.extract_min h1;;
(* Should raise Heap.EmptyHeap *)

let update1 = Heap.insert h1 1 1;;
let update2 = Heap.insert h1 2 2;;
let update3 = Heap.insert h1 3 3;;
let update4 = Heap.insert h1 4 4;;
let update5 = Heap.insert h1 5 5;;

Heap.min h1;;
(* Should be: (1,1) *)
update5 2;;
Heap.min h1;;
(* Should be: (1,1) *)
update4 0;;
Heap.min h1;;
(* Should be: (0,4) *)
Heap.extract_min h1;;
Heap.min h1;;
(* Should be: (1,1) *)
Heap.extract_min h1;;
Heap.min h1;;
(* Should be: (2,2) or (2,5) *)
Heap.extract_min h1;;
Heap.min h1;;
(* Should be: (2,2) or (2,5) *)
Heap.extract_min h1;;
Heap.min h1;;
(* Should be: (1,3) *)
Heap.extract_min h1;;
Heap.min h1;;
(* Should raise Heap.EmptyHeap *)
