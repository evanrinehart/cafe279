import Data.Maybe (mapMaybe)
import Data.List (sort)
import Data.Tuple (swap)

type Point    = Int
type Shape    = [Point]
type Segment  = (Point,Point)

-- 6  7  0
--
-- 5     1
--
-- 4  3  2

wedge :: Shape
wedge = [0,1,2,3,0]

slab :: Shape
slab  = [0,1,2,3,7,0]

rot90 :: Shape -> Shape
rot90 = map (\n -> (n + 2) `mod` 8)

hflip :: Shape -> Shape
hflip = reverse . map mirror

merge :: Shape -> Shape -> Shape
merge = onSegments2 mergeSegs

minus :: Shape -> Shape -> Shape
minus = onSegments2 minusSegs

-- example constructions

vflip = rot90 . rot90 . hflip
empty = wedge `minus` wedge                                     -- []
block = slab `merge` (hflip slab)                               -- [0,1,2,3,4,5,6,7,0]
triangle = hflip r `merge` r where r = slab `minus` vflip wedge -- [2,3,4,7,2]
weird = block `minus` wedge `minus` rot90 (hflip wedge)         -- [0,3,4,5,0]


-- implementation

mirror 0 = 6
mirror 1 = 5
mirror 2 = 4
mirror 3 = 3
mirror 4 = 2
mirror 5 = 1
mirror 6 = 0
mirror 7 = 7

onSegments2 :: ([Segment] -> [Segment] -> [Segment]) -> Shape -> Shape -> Shape
onSegments2 = onIso2 toSegments fromSegments

onIso2 :: (a -> b) -> (b -> a) -> (b -> b -> b) -> a -> a -> a
onIso2 to from f x y = from (f (to x) (to y))

toSegments :: Shape -> [Segment]
toSegments ps = zipWith (,) ps (drop 1 ps)

fromSegments :: [Segment] -> Shape
fromSegments [] = []
fromSegments xs = let (start,_):_ = xs in map fst xs ++ [start]

-- these simple algorithms work for convex shapes only
minusSegs :: [Segment] -> [Segment] -> [Segment]
minusSegs xs ys = sort (s1 ++ s2) where
  s1 = mapMaybe (\pair -> if pair `elem` ys then Nothing else Just pair) xs
  s2 = mapMaybe (\pair -> if pair `elem` xs then Nothing else Just (swap pair)) ys

mergeSegs :: [Segment] -> [Segment] -> [Segment]
mergeSegs xs ys = sort (s1 ++ s2) where
  s1 = mapMaybe (\pair -> if swap pair `elem` ys then Nothing else Just pair) xs
  s2 = mapMaybe (\pair -> if swap pair `elem` xs then Nothing else Just pair) ys

